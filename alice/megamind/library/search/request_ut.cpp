#include "request.h"

#include <alice/megamind/library/search/protos/alice_meta_info.pb.h>

#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/testing/speechkit.h>
#include <alice/megamind/library/testing/speechkit_api_builder.h>

#include <alice/library/experiments/flags.h>
#include <alice/library/logger/logger.h>
#include <alice/library/network/headers.h>
#include <alice/library/unittest/fake_fetcher.h>
#include <alice/library/unittest/mock_sensors.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/string/builder.h>

using namespace NAlice;
using namespace NAlice::NMegamind;
using namespace NAlice::NTestingHelpers;

namespace {

struct TFixture : public NUnitTest::TBaseFixture {
    TFixture() = default;

    TTestSpeechKitRequest BuildSkr(TSpeechKitApiRequestBuilder::EClient client) {
        SkrBuilder.SetTextInput(QueryText);
        SkrBuilder.SetPredefinedClient(client);
        return TSpeechKitRequestBuilder(SkrBuilder.BuildJson()).Build();
    }

    TSourcePrepareStatus
    CreateRequest(TFakeRequestBuilder& builder,
                  TSpeechKitApiRequestBuilder::EClient client = TSpeechKitApiRequestBuilder::EClient::Quasar,
                  const bool imageSearchGranet = false) {
        TWebSearchRequestBuilder webSearchBuilder{QueryText};

        webSearchBuilder.SetUserRegion(GeoId);
        webSearchBuilder.SetContentSettings(ContentSettings);
        webSearchBuilder.SetSensors(Sensors);
        if (imageSearchGranet) {
            webSearchBuilder.SetHasImageSearchGranet();
        }

        const TTestSpeechKitRequest skr = BuildSkr(client);

        return webSearchBuilder.Build(skr, *skr.EventWrapper(), TRTLogger::NullLogger(), builder);
    }

    TSpeechKitApiRequestBuilder SkrBuilder;
    TString QueryText = "hello";
    NGeobase::TId GeoId = 213; // Moscow
    EContentSettings ContentSettings = EContentSettings::without;
    TFakeSensors Sensors;
};

Y_UNIT_TEST_SUITE(SearchRequest) {
    Y_UNIT_TEST_F(Disabled, TFixture) {
        SkrBuilder.EnableExpFlag(ToString(EXP_DISABLE_WEBSEARCH_REQUEST));
        TFakeRequestBuilder builder;
        auto status = CreateRequest(builder);
        UNIT_ASSERT_C(status.IsSuccess(), "must be success because it is not an error when websearch disabled via flag");
        UNIT_ASSERT_C(builder.Headers.Empty(), "headers in disabled request request must be empty");
        UNIT_ASSERT_C(builder.Cgi.empty(), "cgis in disabled request request must be empty");

        const auto* sensor = Sensors.FindFirstRateSensor("name", "websearch_request");
        UNIT_ASSERT_C(sensor, "websearch_request sensor must present");
        UNIT_ASSERT_VALUES_EQUAL_C(sensor->Value, 1, "websearch_request sensor must be 1");
        auto sensorStatus = sensor->Labels.Get("status");
        UNIT_ASSERT_C(sensorStatus.has_value(), "websearch_request sensor must have status label");
        UNIT_ASSERT_VALUES_EQUAL_C(sensorStatus.value()->Value(), "disable",
                                   "websearch_request sensor status label must be disable");
    }

    Y_UNIT_TEST_F(Enabled, TFixture) {
        TFakeRequestBuilder builder;
        auto status = CreateRequest(builder);
        UNIT_ASSERT(status.IsSuccess());

        struct TTestItem {
            TStringBuf Header;
            TMaybe<TStringBuf> Value = {};
        };
        const TTestItem items[] = {
            {
                TStringBuf("x-yandex-internal-flags"),
            },
            {
                TStringBuf("x-yandex-internal-request"),
                TStringBuf("1")
            },
            {
                TStringBuf("x-forwarded-for"),
            },
            {
                TStringBuf("x-forwarded-for-y"),
            },
            {
                TStringBuf("x-real-ip"),
            },
            {
                NAlice::NNetwork::HEADER_X_YANDEX_ALICE_META_INFO,
            },
        };

        for (const auto& item : items) {
            const auto* header = builder.Headers.FindHeader(item.Header);
            UNIT_ASSERT_C(header, TStringBuilder{} << "request must contain a header " << item.Header);
            if (item.Value.Defined()) {
                UNIT_ASSERT_VALUES_EQUAL_C(header->Value(), *item.Value, "header " << item.Header << " must be equal to " << *item.Value);
            } else {
                UNIT_ASSERT_C(!header->Value().Empty(), item.Header << " must not be empty");
            }
        }
        const auto aliceMetaInfoHeader = builder.Headers.FindHeader(NAlice::NNetwork::HEADER_X_YANDEX_ALICE_META_INFO);
        NAlice::TAliceMetaInfo aliceMetaInfo;
        UNIT_ASSERT_C(aliceMetaInfo.ParseFromString(Base64Decode(aliceMetaInfoHeader->Value())), "Failed to parse X-Yandex-Alice-Meta-Info header");
        UNIT_ASSERT_C(!aliceMetaInfo.HasEvent(), "TAliceMetaInfo should not have Event field present");

        auto cgiText = builder.Cgi.Find("text");
        UNIT_ASSERT_C(cgiText != builder.Cgi.end(), "request must contain text param");
        UNIT_ASSERT_VALUES_EQUAL_C(cgiText->second, QueryText, "cgi param text must contain " + QueryText);

        const auto* sensor = Sensors.FindFirstRateSensor("name", "websearch_request");
        UNIT_ASSERT_C(sensor, "websearch_request sensor must present");
        UNIT_ASSERT_VALUES_EQUAL_C(sensor->Value, 1, "websearch_request sensor must be 1");
        auto sensorStatus = sensor->Labels.Get("status");
        UNIT_ASSERT_C(sensorStatus.has_value(), "websearch_request sensor must have status label");
        UNIT_ASSERT_VALUES_EQUAL_C(sensorStatus.value()->Value(), "enable",
                                   "websearch_request sensor status label must be enabled because of quasar client");
    }

    Y_UNIT_TEST_F(DisableAds, TFixture) {
        {
            TFakeRequestBuilder builder;
            SkrBuilder.EnableExpFlag(TString{NAlice::EXP_DISABLE_WEBSEARCH_ADS_FOR_MEGAMIND});
            auto status = CreateRequest(builder);
            UNIT_ASSERT(status.IsSuccess());
            UNIT_ASSERT(builder.Cgi.Has("init_meta", "disable-src-yabs_gallery2"));
            SkrBuilder.DisableExpFlag(TString{NAlice::EXP_DISABLE_WEBSEARCH_ADS_FOR_MEGAMIND});
        }
        {
            TFakeRequestBuilder builder;
            SkrBuilder.EnableExpFlag(TString{NAlice::NExperiments::WEBSEARCH_ENABLE_DIRECT_GALLERY});
            auto status = CreateRequest(builder);
            UNIT_ASSERT(status.IsSuccess());
            UNIT_ASSERT(!builder.Cgi.Has("init_meta", "disable-src-yabs_gallery2"));
            UNIT_ASSERT(builder.Cgi.Has("direct_page", "620060"));
        }
    }

    Y_UNIT_TEST_F(UpperSearchParams, TFixture) {
        {
            TFakeRequestBuilder builder;
            auto status = CreateRequest(builder);
            UNIT_ASSERT(status.IsSuccess());
            const auto* header = builder.Headers.FindHeader(NAlice::NNetwork::HEADER_X_YANDEX_INTERNAL_FLAGS);
            UNIT_ASSERT(header);
            int countInternalHeader = 0;
            for (const auto& header: builder.Headers) {
                if (header.Name() == NNetwork::HEADER_X_YANDEX_INTERNAL_REQUEST) {
                    countInternalHeader++;
                }
            }
            UNIT_ASSERT_VALUES_EQUAL(countInternalHeader, 1);
            UNIT_ASSERT_VALUES_EQUAL(
                header->Value(),
                Base64Encode(
                    R"({"disable_redirects":1,"srcparams":{"UPPER":["rearr=scheme_Local/Facts/Create/EntityAsFactFlag=1","rearr=scheme_Local/Assistant/ClientId=\"ru.yandex.quasar/1.0 ( |  )\"","rearr=scheme_Local/Assistant/ClientIdBase64=\"cnUueWFuZGV4LnF1YXNhci8xLjAgKCB8ICAp\""]},"direct_raw_parameters":"aoff=1"})"));
        }
    }

    Y_UNIT_TEST_F(EnableImageSources, TFixture) {
        {
            TFakeRequestBuilder builder;
            auto status = CreateRequest(builder);
            UNIT_ASSERT(status.IsSuccess());
            UNIT_ASSERT(!builder.Cgi.Has("init_meta", "enable-images-in-alice"));
            UNIT_ASSERT(builder.Cgi.Has("srcskip", "IMAGESP"));
            UNIT_ASSERT(builder.Cgi.Has("srcskip", "IMAGESQUICKP"));
            UNIT_ASSERT(builder.Cgi.Has("srcskip", "IMAGESULTRAP"));
        }
        {
            TFakeRequestBuilder builder;
            SkrBuilder.EnableExpFlag(TString{NAlice::NExperiments::WEBSEARCH_ENABLE_IMAGE_SOURCES});
            auto status = CreateRequest(builder, TSpeechKitApiRequestBuilder::EClient::Quasar, false /* imageSearchGranet */);
            UNIT_ASSERT(status.IsSuccess());
            UNIT_ASSERT(builder.Cgi.Has("init_meta", "enable-images-in-alice"));
            UNIT_ASSERT(!builder.Cgi.Has("srcskip", "IMAGESP"));
            UNIT_ASSERT(!builder.Cgi.Has("srcskip", "IMAGESQUICKP"));
            UNIT_ASSERT(!builder.Cgi.Has("srcskip", "IMAGESULTRAP"));
            SkrBuilder.DisableExpFlag(TString{NAlice::NExperiments::WEBSEARCH_ENABLE_IMAGE_SOURCES});
        }
        {
            TFakeRequestBuilder builder;
            auto status = CreateRequest(builder, TSpeechKitApiRequestBuilder::EClient::Quasar, true /* imageSearchGranet */);
            UNIT_ASSERT(status.IsSuccess());
            UNIT_ASSERT(builder.Cgi.Has("init_meta", "enable-images-in-alice"));
            UNIT_ASSERT(!builder.Cgi.Has("srcskip", "IMAGESP"));
            UNIT_ASSERT(!builder.Cgi.Has("srcskip", "IMAGESQUICKP"));
            UNIT_ASSERT(!builder.Cgi.Has("srcskip", "IMAGESULTRAP"));
        }
    }

    Y_UNIT_TEST_F(ShinyDiscovery, TFixture) {
        TFakeRequestBuilder builder;
        auto status = CreateRequest(builder, TSpeechKitApiRequestBuilder::EClient::Quasar);
        UNIT_ASSERT(status.IsSuccess());

        UNIT_ASSERT(builder.Cgi.Has("rearr", "scheme_Local/ShinyDiscovery/SaasNamespace=shiny_discovery_metadoc_alice"));
        UNIT_ASSERT(builder.Cgi.Has("rearr", "scheme_Local/ShinyDiscovery/InsertMethod=InsertPos"));
    }
}

} // namespace
