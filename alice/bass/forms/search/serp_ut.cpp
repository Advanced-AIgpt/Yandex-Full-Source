#include "serp.h"

#include <alice/bass/ut/helpers.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/network/headers.h>
#include <alice/library/unittest/fake_fetcher.h>

#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NTestingHelpers;
using namespace NBASS;
using namespace NBASS::NSerp;

using EService = NAlice::TWebSearchBuilder::EService;

namespace {

Y_UNIT_TEST_SUITE(TestSerp) {
    Y_UNIT_TEST(IncSearchCounter) {
        {
            auto ctx = NTestingHelpers::MakeContext(NSc::TValue(NTestingHelpers::TRequestJson{}));
            UNIT_ASSERT_VALUES_EQUAL(NSerp::NImpl::IncSearchCounter(*ctx, EService::BassMusic), 1);
            UNIT_ASSERT_VALUES_EQUAL(NSerp::NImpl::IncSearchCounter(*ctx, EService::BassVideo), 1);
            UNIT_ASSERT_VALUES_EQUAL(NSerp::NImpl::IncSearchCounter(*ctx, EService::BassMusic), 2);
            UNIT_ASSERT_VALUES_EQUAL(NSerp::NImpl::IncSearchCounter(*ctx, EService::BassVideo), 2);

            auto newCtx = ctx->SetResponseForm("brave_new_form", false /* setAsCallback */);
            UNIT_ASSERT_VALUES_EQUAL(NSerp::NImpl::IncSearchCounter(*newCtx, EService::BassMusic), 1);
            UNIT_ASSERT_VALUES_EQUAL(NSerp::NImpl::IncSearchCounter(*newCtx, EService::BassVideo), 1);
            UNIT_ASSERT_VALUES_EQUAL(NSerp::NImpl::IncSearchCounter(*newCtx, EService::BassMusic), 2);
            UNIT_ASSERT_VALUES_EQUAL(NSerp::NImpl::IncSearchCounter(*newCtx, EService::BassVideo), 2);

            UNIT_ASSERT_VALUES_EQUAL(NSerp::NImpl::IncSearchCounter(*ctx, EService::BassMusic), 3);
            UNIT_ASSERT_VALUES_EQUAL(NSerp::NImpl::IncSearchCounter(*ctx, EService::Bass), 1);
        }
    }

    Y_UNIT_TEST(AddBassSearchSrcrwr) {
        auto multiRequest = MakeIntrusive<TFakeMultiRequest>(TString{});
        constexpr TStringBuf query = "test";
        const TCgiParameters emptyCgi;

        {
            NTestingHelpers::TRequestJson request;
            request.SetExpFlag(NAlice::NExperiments::EXP_SEARCH_REQUEST_SRCRWR, "UPPER:host.yandex.net:86");
            const auto ctx = NTestingHelpers::MakeContext(NSc::TValue(request));
            auto searchRequest = PrepareSearchRequest(query, *ctx, emptyCgi, EService::Bass, multiRequest);
            TVector<TString> headers = searchRequest->GetHeaders();
            const TString expectedHeader = TString::Join(
                NAlice::NNetwork::HEADER_X_YANDEX_INTERNAL_FLAGS, ": ",
                Base64Encode(
                    R"({"disable_redirects":1,"srcrwr":{"UPPER":"host.yandex.net:86"},"srcparams":{"UPPER":["rearr=scheme_Local/Facts/Create/EntityAsFactFlag=1","rearr=scheme_Local/Assistant/ClientId=\"ru.yandex.searchplugin/7.90 (Xiaomi Redmi Note 4| android 7.1.2)\"","rearr=scheme_Local/Assistant/ClientIdBase64=\"cnUueWFuZGV4LnNlYXJjaHBsdWdpbi83LjkwIChYaWFvbWkgUmVkbWkgTm90ZSA0fCBhbmRyb2lkIDcuMS4yKQ==\""]}})"));
            const auto* header = FindPtr(headers, expectedHeader);
            UNIT_ASSERT(header);
        }
        {
            NTestingHelpers::TRequestJson request;
            request.SetExpFlag(NAlice::NExperiments::EXP_SEARCH_REQUEST_SRCRWR,
                               "UPPER:host.yandex.net:86,MIDDLE:host2.yandex.ru:123?cgi=1,LOWER:host2.yandex-team.ru");
            const auto ctx = NTestingHelpers::MakeContext(NSc::TValue(request));
            auto searchRequest = PrepareSearchRequest(query, *ctx, emptyCgi, EService::Bass, multiRequest);
            TVector<TString> headers = searchRequest->GetHeaders();
            const TString expectedHeader = TString::Join(
                NAlice::NNetwork::HEADER_X_YANDEX_INTERNAL_FLAGS, ": ",
                Base64Encode(
                    R"({"disable_redirects":1,"srcrwr":{"UPPER":"host.yandex.net:86","MIDDLE":"host2.yandex.ru:123?cgi=1","LOWER":"host2.yandex-team.ru"},"srcparams":{"UPPER":["rearr=scheme_Local/Facts/Create/EntityAsFactFlag=1","rearr=scheme_Local/Assistant/ClientId=\"ru.yandex.searchplugin/7.90 (Xiaomi Redmi Note 4| android 7.1.2)\"","rearr=scheme_Local/Assistant/ClientIdBase64=\"cnUueWFuZGV4LnNlYXJjaHBsdWdpbi83LjkwIChYaWFvbWkgUmVkbWkgTm90ZSA0fCBhbmRyb2lkIDcuMS4yKQ==\""]}})"));
            const auto* header = FindPtr(headers, expectedHeader);
            UNIT_ASSERT(header);
        }
        {
            NTestingHelpers::TRequestJson request;
            const auto ctx = NTestingHelpers::MakeContext(NSc::TValue(request));
            auto searchRequest = PrepareSearchRequest(query, *ctx, emptyCgi, EService::Bass, multiRequest);
            TVector<TString> headers = searchRequest->GetHeaders();
            const TString expectedHeader = TString::Join(
                NAlice::NNetwork::HEADER_X_YANDEX_INTERNAL_FLAGS, ": ",
                Base64Encode(
                    R"({"disable_redirects":1,"srcparams":{"UPPER":["rearr=scheme_Local/Facts/Create/EntityAsFactFlag=1","rearr=scheme_Local/Assistant/ClientId=\"ru.yandex.searchplugin/7.90 (Xiaomi Redmi Note 4| android 7.1.2)\"","rearr=scheme_Local/Assistant/ClientIdBase64=\"cnUueWFuZGV4LnNlYXJjaHBsdWdpbi83LjkwIChYaWFvbWkgUmVkbWkgTm90ZSA0fCBhbmRyb2lkIDcuMS4yKQ==\""]}})"));
            const auto* header = FindPtr(headers, expectedHeader);
            UNIT_ASSERT(header);
        }
    }

    Y_UNIT_TEST(AliceReportHashId) {
        auto multiRequest = MakeIntrusive<TFakeMultiRequest>(TString{});
        constexpr TStringBuf query = "test";
        const TCgiParameters emptyCgi;

        {
            const auto ctx = NTestingHelpers::MakeContext(NSc::TValue(NTestingHelpers::TRequestJson{}));
            auto searchRequest = PrepareSearchRequest(query, *ctx, emptyCgi, EService::Bass, multiRequest);
            UNIT_ASSERT(searchRequest->HasHeader(NAlice::NNetwork::HEADER_X_YANDEX_REPORT_ALICE_HASH_ID));
        }
        {
            NTestingHelpers::TRequestJson requestWithFlag;
            requestWithFlag.SetExpFlag(NAlice::NExperiments::WEBSEARCH_DISABLE_REPORT_CACHE);
            const auto ctx = NTestingHelpers::MakeContext(NSc::TValue(requestWithFlag));
            auto searchRequest = PrepareSearchRequest(query, *ctx, emptyCgi, EService::Bass, multiRequest);
            UNIT_ASSERT(!searchRequest->HasHeader(NAlice::NNetwork::HEADER_X_YANDEX_REPORT_ALICE_HASH_ID));
        }
    }

    Y_UNIT_TEST(AliceAdsDisable) {
        auto multiRequest = MakeIntrusive<TFakeMultiRequest>(TString{});
        constexpr TStringBuf query = "test";
        const TCgiParameters emptyCgi;

        {
            const auto ctx = NTestingHelpers::MakeContext(NSc::TValue(NTestingHelpers::TRequestJson{}));
            auto searchRequest = PrepareSearchRequest(query, *ctx, emptyCgi, EService::Bass, multiRequest);
            UNIT_ASSERT(searchRequest->Url().find("init_meta=disable-src-yabs_distr&init_meta=disable-src-yabs_exp") ==
                        TString::npos);
        }
        {
            const auto ctx = NTestingHelpers::MakeContext(NSc::TValue(NTestingHelpers::TRequestJson{}));
            auto searchRequest = PrepareSearchRequest(query, *ctx, emptyCgi, EService::BassVideo, multiRequest);
            UNIT_ASSERT(searchRequest->Url().find("init_meta=disable-src-yabs_distr&init_meta=disable-src-yabs_exp") !=
                        TString::npos);
        }
        {
            NTestingHelpers::TRequestJson requestWithFlag;
            requestWithFlag.SetExpFlag(NAlice::NExperiments::WEBSEARCH_ENABLE_ADS_FOR_NONSEARCH);
            const auto ctx = NTestingHelpers::MakeContext(NSc::TValue(requestWithFlag));
            auto searchRequest = PrepareSearchRequest(query, *ctx, emptyCgi, EService::BassVideo, multiRequest);
            UNIT_ASSERT(searchRequest->Url().find("init_meta=disable-src-yabs_distr&init_meta=disable-src-yabs_exp") ==
                        TString::npos);
        }
    }

    Y_UNIT_TEST(AliceDirect) {
        auto multiRequest = MakeIntrusive<TFakeMultiRequest>(TString{});
        constexpr TStringBuf query = "test";
        const TCgiParameters emptyCgi;

        {
            auto json = NTestingHelpers::TRequestJson{}.SetClient("ru.yandex.searchplugin").SetUID(12345);
            const auto ctx = NTestingHelpers::MakeContext(NSc::TValue(json));
            auto searchRequest = PrepareSearchRequest(query, *ctx, emptyCgi, EService::Bass, multiRequest);
            TVector<TString> headers = searchRequest->GetHeaders();
            const TString expectedHeader = TString::Join(
                NAlice::NNetwork::HEADER_X_YANDEX_INTERNAL_FLAGS, ": ",
                Base64Encode(
                    R"({"disable_redirects":1,"srcparams":{"UPPER":["rearr=scheme_Local/Facts/Create/EntityAsFactFlag=1","rearr=scheme_Local/Assistant/ClientId=\"ru.yandex.searchplugin\"","rearr=scheme_Local/Assistant/ClientIdBase64=\"cnUueWFuZGV4LnNlYXJjaHBsdWdpbg==\""]}})"));
            const auto* header = FindPtr(headers, expectedHeader);
            UNIT_ASSERT(header);
            UNIT_ASSERT(searchRequest->Url().find("direct_page=620060") != TString::npos);
        }
        {
            auto json = NTestingHelpers::TRequestJson{}.SetClient("ru.yandex.searchplugin").SetUID(12345);
            const auto ctx = NTestingHelpers::MakeContext(NSc::TValue(json));
            auto searchRequest = PrepareSearchRequest(query, *ctx, emptyCgi, EService::BassVideo, multiRequest);
            TVector<TString> headers = searchRequest->GetHeaders();
            const TString expectedHeader =
                TString::Join(NAlice::NNetwork::HEADER_X_YANDEX_INTERNAL_FLAGS, ": ",
                              Base64Encode(R"({"disable_redirects":1,"direct_raw_parameters":"aoff=1"})"));
            const auto* header = FindPtr(headers, expectedHeader);
            UNIT_ASSERT(header);
            UNIT_ASSERT(searchRequest->Url().find("direct_page") == TString::npos);
        }
        {
            auto json = NTestingHelpers::TRequestJson{}
                            .SetClient("ru.yandex.searchplugin")
                            .SetUID(12345)
                            .SetExpFlag("direct_expid_for_search=1111");
            const auto ctx = NTestingHelpers::MakeContext(NSc::TValue(json));
            auto searchRequest = PrepareSearchRequest(query, *ctx, emptyCgi, EService::Bass, multiRequest);
            UNIT_ASSERT(searchRequest->Url().find("exp_flags=direct_raw_parameters%3Dexperiment-id%3D1111") !=
                        TString::npos);
        }
        {
            auto json = NTestingHelpers::TRequestJson{}
                            .SetClient("ru.yandex.searchplugin")
                            .SetUID(12345)
                            .SetExpFlag("direct_page=22");
            const auto ctx = NTestingHelpers::MakeContext(NSc::TValue(json));
            auto searchRequest = PrepareSearchRequest(query, *ctx, emptyCgi, EService::Bass, multiRequest);
            TVector<TString> headers = searchRequest->GetHeaders();
            UNIT_ASSERT(searchRequest->Url().find("direct_page=22") != TString::npos);
        }
        {
            auto json = NTestingHelpers::TRequestJson{}
                            .SetClient("ru.yandex.searchplugin")
                            .SetUID(12345)
                            .SetExpFlag("enable_direct_in_wizard");
            const auto ctx = NTestingHelpers::MakeContext(NSc::TValue(json));
            auto searchRequest = PrepareSearchRequest(query, *ctx, emptyCgi, EService::Bass, multiRequest);
            TVector<TString> headers = searchRequest->GetHeaders();
            UNIT_ASSERT(searchRequest->Url().find("direct_page=620060") != TString::npos);
        }
        {
            auto json = NTestingHelpers::TRequestJson{}
                            .SetClient("ru.yandex.searchplugin")
                            .SetUID(12345)
                            .SetExpFlag("disable_ads_in_bass_search");
            const auto ctx = NTestingHelpers::MakeContext(NSc::TValue(json));
            auto searchRequest = PrepareSearchRequest(query, *ctx, emptyCgi, EService::Bass, multiRequest);
            TVector<TString> headers = searchRequest->GetHeaders();
            UNIT_ASSERT(searchRequest->Url().find("direct_page") == TString::npos);
        }
    }

    Y_UNIT_TEST(EncodedAliceMeta) {
        auto multiRequest = MakeIntrusive<TFakeMultiRequest>(TString{});
        constexpr TStringBuf query = "test";
        const TCgiParameters emptyCgi;

        TString encodedAliceMeta;

        const auto ctx = NTestingHelpers::MakeContext(TStringBuf(R"json({
            "form": {
                "name": "personal_assistant.scenarios.music_play"
            },
            "meta": {
                "epoch": 1504271099,
                "tz": "UTC",
                "uuid": "00000000-0000-0000-0000-000000000000",
                "utterance": "hello",
                "uid": 4007095345,
                "client_id": "ru.yandex.searchplugin.dev/7.10 (none none; android 7.1.2)",
                "user_agent": "Mozilla/5.0 (Linux; Android 7.1.2; Redmi Note 4 Build/NJH47F; wv) AppleWebKit/537.36 (KHTML, like Gecko) Version/4.0 Chrome/63.0.3239.111 Mobile Safari/537.36 YandexSearch/7.10",
                "event": {
                    "type": "voice_input",
                    "BiometryClassification": {
                        "status": "ok"
                    }
                }
            }
        })json"));
        auto searchRequest = PrepareMusicSearchRequest(query, *ctx, emptyCgi, multiRequest, encodedAliceMeta);
        UNIT_ASSERT(searchRequest->HasHeader(NAlice::NNetwork::HEADER_X_YANDEX_ALICE_META_INFO));
        UNIT_ASSERT(!encodedAliceMeta.empty());
        UNIT_ASSERT_NO_EXCEPTION(Base64Decode(encodedAliceMeta));
    }

    Y_UNIT_TEST(ShinyDiscovery) {
        for (const TStringBuf client : {"ru.yandex.quasar", "com.yandex.tv.alice"}) {
            NTestingHelpers::TRequestJson request;
            request.SetClient(client);

            const auto ctx = NTestingHelpers::MakeContext(NSc::TValue(request));

            auto searchRequest = PrepareSearchRequest(
                "сколько ног у паука",
                *ctx,
                TCgiParameters{},
                EService::Bass,
                MakeIntrusive<TFakeMultiRequest>(TString{})
            );

            UNIT_ASSERT(searchRequest->Url().find(
                "rearr=scheme_Local/ShinyDiscovery/SaasNamespace%3Dshiny_discovery_metadoc_alice") != TString::npos);

            UNIT_ASSERT(searchRequest->Url().find(
                "rearr=scheme_Local/ShinyDiscovery/InsertMethod%3DInsertPos") != TString::npos);
        }
    }

    Y_UNIT_TEST(ReqInfoClientName) {
        for (const TStringBuf client : {"ru.yandex.quasar", "ru.yandex.searchplugin"}) {
            NTestingHelpers::TRequestJson request;
            request.SetClient(client);

            const auto ctx = NTestingHelpers::MakeContext(NSc::TValue(request));

            auto searchRequest = PrepareSearchRequest(
                "dummy",
                *ctx,
                TCgiParameters{},
                EService::Bass,
                MakeIntrusive<TFakeMultiRequest>(TString{})
            );

            const TString expectedReqInfo = TString::Join("&reqinfo=assistant.yandex.", client);
            UNIT_ASSERT(searchRequest->Url().find(expectedReqInfo) != TString::npos);
        }
    }
}

} // namespace
