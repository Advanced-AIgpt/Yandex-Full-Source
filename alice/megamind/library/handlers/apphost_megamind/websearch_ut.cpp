#include "websearch.h"

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/apphost_request/util.h>
#include <alice/megamind/library/testing/apphost_helpers.h>
#include <alice/megamind/library/testing/mock_context.h>
#include <alice/megamind/library/testing/speechkit.h>
#include <alice/megamind/library/testing/speechkit_api_builder.h>

#include <alice/library/json/json.h>
#include <alice/library/scenarios/data_sources/data_sources.h>
#include <alice/library/unittest/fake_fetcher.h>
#include <alice/library/unittest/message_diff.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/string/builder.h>

namespace {

using namespace NAlice;
using namespace NAlice::NMegamind;
using namespace NAlice::NMegamind::NTesting;
using namespace testing;

Y_UNIT_TEST_SUITE(AppHostMegamindWebSearch) {
    Y_UNIT_TEST_F(SetupSmoke, TAppHostFixture) {
        GlobalCtx.GenericInit();

        auto ahCtx = CreateAppHostContext();

        TSpeechKitApiRequestBuilder skrApiBuilder;
        skrApiBuilder.SetTextInput("hello");
        auto skr = TSpeechKitRequestBuilder{skrApiBuilder.BuildJson()}.Build();

        NAlice::TEvent eventProto;
        eventProto.SetText("hello");
        eventProto.SetType(NAlice::EEventType::text_input);
        auto event = IEvent::CreateEvent(eventProto);

        TWebSearchRequestBuilder webRequestBuilder{"covid"};
        const auto status = AppHostWebSearchSetup(ahCtx, skr, *event, webRequestBuilder);
        UNIT_ASSERT_C(!status.Defined(), TStringBuilder{} << "websearchsetup error: " << status);
        UNIT_ASSERT_C(ahCtx.TestCtx().HasProtobufItem(AH_ITEM_WEBSEARCH_HTTP_REQUEST_NAME), "does't have websearch request proto");
    }

    Y_UNIT_TEST_F(SetupEmptyText, TAppHostFixture) {
        GlobalCtx.GenericInit();

        auto ahCtx = CreateAppHostContext();

        TSpeechKitApiRequestBuilder skrApiBuilder;
        skrApiBuilder.SetTextInput(TString{});
        auto skr = TSpeechKitRequestBuilder{skrApiBuilder.BuildJson()}.Build();

        NAlice::TEvent eventProto;
        eventProto.SetText(TString{});
        eventProto.SetType(NAlice::EEventType::text_input);
        auto event = IEvent::CreateEvent(eventProto);

        TWebSearchRequestBuilder webRequestBuilder{TString{}};
        const auto status = AppHostWebSearchSetup(ahCtx, skr, *event, webRequestBuilder);
        UNIT_ASSERT_C(!status.Defined(), TStringBuilder{} << "websearchsetup error: " << status);
        UNIT_ASSERT_C(!ahCtx.TestCtx().HasProtobufItem(AH_ITEM_WEBSEARCH_HTTP_REQUEST_NAME),
                      "Shouldn't create websearch request for empty utterance");
    }

    Y_UNIT_TEST_F(TestGetPartedSearchResponse, TAppHostFixture) {
        GlobalCtx.GenericInit();

        auto ahCtx = CreateAppHostContext();

        NMegamind::TMockInitializer initializer;

        const auto TUNNELLER_RAW_RESPONSE = "tunneller raw response";
        {
            NWebSearch::TTunnellerRawResponse response;
            response.MutableRawResponse()->set_value(TUNNELLER_RAW_RESPONSE);
            ahCtx.TestCtx().AddProtobufItem(response, AH_ITEM_SEARCH_PART_TUNNELLER_RAW_RESPONSE, NAppHost::EContextItemKind::Input);
        }

        const TVector<float> FACTORS = {0.123, 0.321, 11.99};
        {
            NWebSearch::TStaticBlenderFactors response;
            for (const auto factor : FACTORS) {
                response.AddFactors(factor);
            }
            ahCtx.TestCtx().AddProtobufItem(response, AH_ITEM_SEARCH_PART_STATIC_BLENDER_FACTORS, NAppHost::EContextItemKind::Input);
        }

        const TVector<TString>  RAW_GROUPING_RESPONSE = {
            R"({"ha":"lol"})",
            R"({"kek":"rofl"})",
        };
        {
            NWebSearch::TReportGrouping response;
            for (const auto& rg : RAW_GROUPING_RESPONSE) {
                response.AddRawGrouping(rg);
            }
            ahCtx.TestCtx().AddProtobufItem(response, AH_ITEM_SEARCH_PART_REPORT_GROUPING, NAppHost::EContextItemKind::Input);
        }

        const NJson::TJsonValue RAW_DOCS = NAlice::JsonFromString(R"([{"type":"x1"},{"type":"x2"}])");
        {
            NScenarios::TDataSource dataSource;
            auto& webSearchDocs = *dataSource.MutableWebSearchDocs();
            for (const auto& rg : RAW_DOCS.GetArray()) {
                auto& doc = *webSearchDocs.AddDocs();
                doc.SetType(rg["type"].GetString());
            }
            const TString& webSearchDocsItemName = NScenarios::GetDataSourceContextName(EDataSourceType::WEB_SEARCH_DOCS);
            ahCtx.TestCtx().AddProtobufItem(dataSource, webSearchDocsItemName, NAppHost::EContextItemKind::Input);
        }

        NAlice::TSearchResponse::TBgFactors BG_FACTORS;
        BG_FACTORS.SetDmozQueryThemes(0.113);
        {
            ahCtx.TestCtx().AddProtobufItem(BG_FACTORS, AH_ITEM_SEARCH_PART_BG_FACTORS, NAppHost::EContextItemKind::Input);
        }

        const auto result = NAlice::NMegamind::AppHostWebSearchPostSetup(ahCtx);

        UNIT_ASSERT(*result.GetTunnellerRawResponse() == TUNNELLER_RAW_RESPONSE);
        const auto& factors = *result.StaticBlenderFactors();
        UNIT_ASSERT(factors.size() == FACTORS.size());
        for (size_t i = 0; i < FACTORS.size(); ++i) {
            UNIT_ASSERT(factors[i] == FACTORS[i]);
        }
        const auto& grouping = result.GetReportGrouping();
        UNIT_ASSERT(grouping.size() == RAW_GROUPING_RESPONSE.size());
        for (size_t i = 0; i < RAW_GROUPING_RESPONSE.size(); ++i) {
            UNIT_ASSERT(grouping[i].ToJson() == RAW_GROUPING_RESPONSE[i]);
        }
        const auto& docs = result.Docs();
        UNIT_ASSERT(docs.GetDocs().size() == static_cast<int>(RAW_DOCS.GetArray().size()));
        for (size_t i = 0; i < RAW_DOCS.GetArray().size(); ++i) {
            UNIT_ASSERT(NAlice::JsonStringFromProto(docs.GetDocs()[i]) == NAlice::JsonToString(RAW_DOCS[i]));
        }
        UNIT_ASSERT_MESSAGES_EQUAL(BG_FACTORS, *result.BgFactors());
   }
}

} // namespace
