#include "websearch.h"

#include <alice/library/network/headers.h>

#include <alice/library/unittest/fake_fetcher.h>

#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace NAlice;
using namespace NAlice::NTestingHelpers;

using EService = NAlice::TWebSearchBuilder::EService;

void TestBassExpFlags(EService service) {
    TFakeRequestBuilder request;
    TWebSearchBuilder builder(/* searchUi= */ "quasar", service, /* isChildMode= */ false, /* addInitHeader= */ false);

    const TString origCgi = request.Cgi.Print();

    builder.OnExpFlag("websearch_cgi_rearr=123", Nothing());
    builder.Flush(request);
    UNIT_ASSERT_VALUES_EQUAL_C(origCgi, request.Cgi.Print(), "Value is nothing");

    builder.OnExpFlag("websearch_cgi_rearr=123", "some value");
    builder.Flush(request);
    UNIT_ASSERT_VALUES_EQUAL_C(origCgi, request.Cgi.Print(), "Value is not '1'");

    builder.OnExpFlag("websearch_mm_cgi_rearr=123", "1");
    builder.Flush(request);
    UNIT_ASSERT_VALUES_EQUAL_C(origCgi, request.Cgi.Print(), "Cgi is for MM");

    builder.OnExpFlag("websearch_bass_video_cgi_rearr=123", "1");
    builder.Flush(request);
    UNIT_ASSERT_VALUES_EQUAL_C(origCgi, request.Cgi.Print(), "Cgi is for Video");

    builder.OnExpFlag("websearch_cgi_rearr=123", "1");
    builder.Flush(request);
    UNIT_ASSERT_UNEQUAL(origCgi, request.Cgi.Print());
    UNIT_ASSERT_VALUES_EQUAL(request.Cgi.Get("rearr"), "123");

    builder.OnExpFlag("websearch_bass_cgi_relev=234", "1");
    builder.Flush(request);
    UNIT_ASSERT_UNEQUAL(origCgi, request.Cgi.Print());
    UNIT_ASSERT_VALUES_EQUAL(request.Cgi.Get("relev"), "234");
}

void TestBassVideoExpFlags(EService service) {
    TFakeRequestBuilder request;
    TWebSearchBuilder builder(/* searchUi= */ "quasar", service, /* isChildMode= */ false, /* addInitHeader= */ false);

    const TString origCgi = request.Cgi.Print();

    builder.OnExpFlag("websearch_cgi_rearr=123", Nothing());
    builder.Flush(request);
    UNIT_ASSERT_VALUES_EQUAL_C(origCgi, request.Cgi.Print(), "Value is nothing");

    builder.OnExpFlag("websearch_cgi_rearr=123", "some value");
    builder.Flush(request);
    UNIT_ASSERT_VALUES_EQUAL_C(origCgi, request.Cgi.Print(), "Value is not '1'");

    builder.OnExpFlag("websearch_mm_cgi_rearr=123", "1");
    builder.Flush(request);
    UNIT_ASSERT_VALUES_EQUAL_C(origCgi, request.Cgi.Print(), "Cgi is for MM");

    builder.OnExpFlag("websearch_cgi_rearr=123", "1");
    builder.Flush(request);
    UNIT_ASSERT_VALUES_EQUAL_C(origCgi, request.Cgi.Print(), "Cgi is for web search");

    builder.OnExpFlag("websearch_bass_cgi_relev=234", "1");
    builder.Flush(request);
    UNIT_ASSERT_VALUES_EQUAL_C(origCgi, request.Cgi.Print(), "Cgi is for web search");

    builder.OnExpFlag("websearch_bass_video_cgi_rearr=123", "1");
    builder.Flush(request);
    UNIT_ASSERT_UNEQUAL(origCgi, request.Cgi.Print());
    UNIT_ASSERT_VALUES_EQUAL(request.Cgi.Get("rearr"), "123");
}

void TestReportHash(TStringBuf query, TStringBuf requestId, EService service, TStringBuf expectedHeader) {
    TFakeRequestBuilder request;
    TWebSearchBuilder builder(/* searchUi= */ "quasar", service, /* isChildMode= */ false, /* addInitHeader= */ false);
    builder.AddCgiParam("text", query);
    builder.AddReportHashId(requestId, "1234", EReportCacheMode::Prod);
    builder.Flush(request);

    const THttpInputHeader* header = request.Headers.FindHeader(NNetwork::HEADER_X_YANDEX_REPORT_ALICE_HASH_ID);
    UNIT_ASSERT(header);
    UNIT_ASSERT_VALUES_EQUAL(expectedHeader, header->Value());
}

TString GetReportPriemkaHash(
    TStringBuf requestId,
    const TVector<std::pair<TString, TString>>& cgi,
    const TVector<std::pair<TString, TString>>& headers,
    TMaybe<TStringBuf> seed = Nothing())
{
    TFakeRequestBuilder request;
    TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ false, /* addInitHeader= */ false);

    for (const auto& [key, value] : cgi) {
        builder.AddCgiParam(key, value);
    }
    for (const auto& [key, value] : headers) {
        builder.AddHeader(key, value);
    }

    TReportCacheFlags flags = EReportCacheMode::Priemka;
    if (seed.Defined()) {
        flags |= EReportCacheMode::UseSeed;
    }

    builder.AddReportHashId(requestId, seed.GetOrElse("345"), flags);
    builder.Flush(request);

    const THttpInputHeader* header = request.Headers.FindHeader(NNetwork::HEADER_X_YANDEX_REPORT_ALICE_HASH_ID);

    UNIT_ASSERT(header);
    return header->Value();
}

Y_UNIT_TEST_SUITE(WebSearch) {
    Y_UNIT_TEST(GenericFixupsSmoke) {
        TFakeRequestBuilder request;
        TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ false, /* addInitHeader= */ false);
        builder.GenericFixups();
        builder.Flush(request);
        UNIT_ASSERT(request.ContentType.Defined());
        UNIT_ASSERT(!request.ContentType->Empty());
        UNIT_ASSERT(request.Cgi.Has("flags", "blender_tmpl_data=1"));
        UNIT_ASSERT(request.Cgi.Has("rearr", "scheme_Local/Assistant/ClientCanShowSerp=0"));
        UNIT_ASSERT(request.HasHeader(NNetwork::HEADER_X_YANDEX_INTERNAL_REQUEST));
        UNIT_ASSERT_VALUES_EQUAL(request.Cgi.NumOfValues("rearr"), 7);
    }

    Y_UNIT_TEST(ExpFlagsSmoke) {
        TFakeRequestBuilder request;
        TWebSearchBuilder bass(/* searchUi= */ "quasar", EService::Bass, /* isChildMode= */ false, /* addInitHeader= */ false);

        const TString origCgi = request.Cgi.Print();
        bass.OnExpFlag("flag1", Nothing());
        bass.OnExpFlag("flag2=123", Nothing());
        bass.OnExpFlag("flag3", "1");
        bass.Flush(request);
        UNIT_ASSERT_VALUES_EQUAL(origCgi, request.Cgi.Print());
    }

    Y_UNIT_TEST(ExpFlagsToRequestForBassAndNavi) {
        TestBassExpFlags(EService::Bass);
        TestBassExpFlags(EService::BassNavigation);
    }

    Y_UNIT_TEST(ExpFlagsToRequestForBassVideo) {
        TestBassVideoExpFlags(EService::BassVideo);
        TestBassVideoExpFlags(EService::BassVideoHost);
    }

    Y_UNIT_TEST(ExpFlagsToRequestForMM) {
        TFakeRequestBuilder request;
        TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ false, /* addInitHeader= */ false);

        const TString origCgi = request.Cgi.Print();

        builder.OnExpFlag("websearch_cgi_rearr=123", Nothing());
        builder.Flush(request);
        UNIT_ASSERT_VALUES_EQUAL_C(origCgi, request.Cgi.Print(), "Value is nothing");

        builder.OnExpFlag("websearch_cgi_rearr=123", "some value");
        builder.Flush(request);
        UNIT_ASSERT_VALUES_EQUAL_C(origCgi, request.Cgi.Print(), "Value is not '1'");

        builder.OnExpFlag("websearch_bass_cgi_rearr=123", "1");
        builder.Flush(request);
        UNIT_ASSERT_VALUES_EQUAL_C(origCgi, request.Cgi.Print(), "Cgi is for BASS");

        builder.OnExpFlag("websearch_cgi_rearr=123", "1");
        builder.Flush(request);
        UNIT_ASSERT_UNEQUAL(origCgi, request.Cgi.Print());
        UNIT_ASSERT_VALUES_EQUAL(request.Cgi.Get("rearr"), "123");

        builder.OnExpFlag("websearch_mm_cgi_relev=234", "1");
        builder.Flush(request);
        UNIT_ASSERT_UNEQUAL(origCgi, request.Cgi.Print());
        UNIT_ASSERT_VALUES_EQUAL(request.Cgi.Get("relev"), "234");
    }

    Y_UNIT_TEST(ICookieValid) {
        TFakeRequestBuilder request;
        TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ false, /* addInitHeader= */ false);

        builder.SetUuid("deadbeefdeadbeefdeadbeef10000008");
        builder.Flush(request);

        const THttpInputHeader* header = request.Headers.FindHeader(NNetwork::HEADER_X_YANDEX_ICOOKIE);
        UNIT_ASSERT(header);
        UNIT_ASSERT(header->Value().Size());
    }

    Y_UNIT_TEST(ICookieInvalid) {
        TFakeRequestBuilder request;
        TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ false, /* addInitHeader= */ false);

        builder.SetUuid("invalid_cookie");
        builder.Flush(request);

        const THttpInputHeader* header = request.Headers.FindHeader(NNetwork::HEADER_X_YANDEX_ICOOKIE);
        UNIT_ASSERT(!header);
    }

    Y_UNIT_TEST(SetUuidSmoke) {
        TFakeRequestBuilder request;
        TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ false, /* addInitHeader= */ false);

        builder.SetUuid("deadbeefdeadbeefdeadbeef10000008");
        builder.Flush(request);

        auto it = request.Cgi.Find("uuid");
        UNIT_ASSERT(it != request.Cgi.end());
        UNIT_ASSERT_VALUES_EQUAL(it->second, "deadbeefdeadbeefdeadbeef10000008");
    }

    Y_UNIT_TEST(SetUuidICookieFeature) {
        {
            TFakeRequestBuilder request;
            TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ false, /* addInitHeader= */ false);
            builder.Features().SetICookieCallback([]() { return false; });
            builder.SetUuid("deadbeefdeadbeefdeadbeef10000008");
            builder.Flush(request);

            const THttpInputHeader* header = request.Headers.FindHeader(NNetwork::HEADER_X_YANDEX_ICOOKIE);
            UNIT_ASSERT_C(!header, "icookie header must not be there! callback returns false.");
        }

        {
            TFakeRequestBuilder request;
            TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ false, /* addInitHeader= */ false);
            builder.Features().SetICookieCallback([]() { return true; });
            builder.SetUuid("deadbeefdeadbeefdeadbeef10000008");
            builder.Flush(request);

            const THttpInputHeader* header = request.Headers.FindHeader(NNetwork::HEADER_X_YANDEX_ICOOKIE);
            UNIT_ASSERT_C(header, "icookie header must be there, callback returns true");
            UNIT_ASSERT(header->Value().Size());
        }
    }

    Y_UNIT_TEST(SetSkipImageSourcesFeature) {
        {
            TFakeRequestBuilder request;
            TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ false, /* addInitHeader= */ false);

            builder.Flush(request);
            UNIT_ASSERT_VALUES_EQUAL(request.Cgi.NumOfValues("srcskip"), 0);

            builder.GenericFixups();
            builder.Flush(request);
            UNIT_ASSERT_VALUES_EQUAL(request.Cgi.NumOfValues("srcskip"), 3);
        }
        {
            TFakeRequestBuilder request;
            TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ false, /* addInitHeader= */ false);

            builder.Flush(request);
            UNIT_ASSERT_VALUES_EQUAL(request.Cgi.NumOfValues("srcskip"), 0);

            builder.Features().SetSkipImageSourcesCallback([]() { return true; });
            builder.GenericFixups();
            builder.Flush(request);
            UNIT_ASSERT_VALUES_EQUAL(request.Cgi.NumOfValues("srcskip"), 3);
        }
        {
            TFakeRequestBuilder request;
            TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ false, /* addInitHeader= */ false);
            builder.Features().SetSkipImageSourcesCallback([]() { return false; });
            builder.GenericFixups();
            builder.Flush(request);

            UNIT_ASSERT_VALUES_EQUAL(request.Cgi.NumOfValues("srcskip"), 0);
        }
    }

    Y_UNIT_TEST(SetServiceTicket) {
        TFakeRequestBuilder request;
        TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ false, /* addInitHeader= */ false);

        constexpr TStringBuf expectedUserTicket = "hahaha";
        builder.SetServiceTicket(expectedUserTicket);
        builder.Flush(request);

        const THttpInputHeader* header = request.Headers.FindHeader(NNetwork::HEADER_X_YA_SERVICE_TICKET);
        UNIT_ASSERT(header);
        UNIT_ASSERT_VALUES_EQUAL(expectedUserTicket, header->Value());
    }

    Y_UNIT_TEST(SetUserTicket) {
        TFakeRequestBuilder request;
        TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ false, /* addInitHeader= */ false);

        constexpr TStringBuf expectedUserTicket = "hahaha";
        builder.SetUserTicket(expectedUserTicket);
        builder.Flush(request);

        const THttpInputHeader* header = request.Headers.FindHeader(NNetwork::HEADER_X_YA_USER_TICKET);
        UNIT_ASSERT(header);
        UNIT_ASSERT_VALUES_EQUAL(expectedUserTicket, header->Value());
    }

    Y_UNIT_TEST(AddProdReportHashId) {
        constexpr TStringBuf query = "test";
        constexpr TStringBuf requestId = "ffffffff-ffff-ffff-ffff-ffffffffffff";
        constexpr TStringBuf expectedBassHash = "5BB9A7B67B1AEC250BD7CC8B9C711B0903A150C9D52238CDB5CBAC2756B83DCD";

        TestReportHash(query, requestId, EService::Megamind, expectedBassHash);
        TestReportHash(query, requestId, EService::Bass, expectedBassHash);
        TestReportHash(query, requestId, EService::BassNavigation, expectedBassHash);

        TestReportHash(query, requestId, EService::BassNews,
                       "938A93C2B65603E9F32B01C854E0F1FAAF244E159C3CC76E4EB5F890A2CFE60A");
        TestReportHash(query, requestId, EService::BassMusic,
                       "F83FE88D1F27CABA06A2285A53C16826EE1A19D98564C62ADB20EBB9D93A08EF");
        TestReportHash(query, requestId, EService::BassVideo,
                       "5E43673F0D9BEA852CD7412ADD6490678A38F7BA6DD1544424CB4BB6A64A55BF");
        TestReportHash(query, requestId, EService::BassVideoHost,
                       "EB348FEFDE7613747B2454B273ED519417883039E485E406F4855C574005F9C5");
    }

    Y_UNIT_TEST(GetReportCacheFlags) {
        {
            const auto flags = GetReportCacheFlags("priemka");
            UNIT_ASSERT(flags & EReportCacheMode::Priemka);
            UNIT_ASSERT(!(flags & EReportCacheMode::Prod));
            UNIT_ASSERT(!(flags & EReportCacheMode::DisableLookup));
            UNIT_ASSERT(!(flags & EReportCacheMode::DisablePut));
        }

        {
            const auto flags = GetReportCacheFlags("prod");
            UNIT_ASSERT(!(flags & EReportCacheMode::Priemka));
            UNIT_ASSERT(flags & EReportCacheMode::Prod);
            UNIT_ASSERT(!(flags & EReportCacheMode::DisableLookup));
            UNIT_ASSERT(!(flags & EReportCacheMode::DisablePut));
        }

        {
            const auto flags = GetReportCacheFlags("priemka,disable-put");
            UNIT_ASSERT(flags & EReportCacheMode::Priemka);
            UNIT_ASSERT(!(flags & EReportCacheMode::Prod));
            UNIT_ASSERT(!(flags & EReportCacheMode::DisableLookup));
            UNIT_ASSERT(flags & EReportCacheMode::DisablePut);
        }

        {
            const auto flags = GetReportCacheFlags("priemka,disable-lookup");
            UNIT_ASSERT(flags & EReportCacheMode::Priemka);
            UNIT_ASSERT(!(flags & EReportCacheMode::Prod));
            UNIT_ASSERT(flags & EReportCacheMode::DisableLookup);
            UNIT_ASSERT(!(flags & EReportCacheMode::DisablePut));
        }

        {
            const auto flags = GetReportCacheFlags("priemka,disable-put,disable-lookup");
            UNIT_ASSERT(flags & EReportCacheMode::Priemka);
            UNIT_ASSERT(!(flags & EReportCacheMode::Prod));
            UNIT_ASSERT(flags & EReportCacheMode::DisableLookup);
            UNIT_ASSERT(flags & EReportCacheMode::DisablePut);
        }

        {
            const auto flags = GetReportCacheFlags(Nothing());
            UNIT_ASSERT(!(flags & EReportCacheMode::Priemka));
            UNIT_ASSERT(flags & EReportCacheMode::Prod);
            UNIT_ASSERT(!(flags & EReportCacheMode::DisableLookup));
            UNIT_ASSERT(!(flags & EReportCacheMode::DisablePut));
        }
    }

    Y_UNIT_TEST(AddPriemkaReportHashId) {
        constexpr TStringBuf requestId = "ffffffff-ffff-ffff-ffff-ffffffffffff";
        constexpr TStringBuf seed = "qwerty";

        UNIT_ASSERT_VALUES_UNEQUAL(GetReportPriemkaHash(requestId, {{"a", "1"}}, {}), GetReportPriemkaHash(requestId, {{"a", "2"}}, {}));
        UNIT_ASSERT_VALUES_EQUAL(GetReportPriemkaHash(requestId, {{"a", "1"}}, {}), GetReportPriemkaHash(requestId, {{"a", "1"}}, {}));
        UNIT_ASSERT_VALUES_UNEQUAL(GetReportPriemkaHash(requestId, {{"a", "1"}}, {}), GetReportPriemkaHash(requestId, {{"b", "1"}}, {}));
        UNIT_ASSERT_VALUES_EQUAL(GetReportPriemkaHash(requestId, {{"a", "1"}}, {}), GetReportPriemkaHash(requestId, {{"a", "1"}, {"uuid", "42"}}, {}));
        UNIT_ASSERT_VALUES_EQUAL(GetReportPriemkaHash(requestId, {{"a", "1"}}, {}), GetReportPriemkaHash(requestId, {{"a", "1"}, {"reqinfo", "qwe"}}, {}));
        UNIT_ASSERT_VALUES_UNEQUAL(GetReportPriemkaHash(requestId, {{"a", "1"}}, {}), GetReportPriemkaHash(requestId, {{"a", "1"}}, {{"X-Yandex-Internal-Flags", "OLOLO"}}));
        UNIT_ASSERT_VALUES_EQUAL(GetReportPriemkaHash(requestId, {{"a", "1"}}, {}), GetReportPriemkaHash(requestId, {{"a", "1"}}, {{"X-Yandex-Internal-Glagne", "OLOLO"}}));
        UNIT_ASSERT_VALUES_UNEQUAL(GetReportPriemkaHash(requestId, {{"a", "1"}}, {}, "1234"), GetReportPriemkaHash(requestId, {{"a", "1"}}, {}, "567"));

        for (auto service : {EService::Megamind, EService::Bass}) {
            TFakeRequestBuilder request;
            TWebSearchBuilder builder(/* searchUi= */ "quasar", service, /* isChildMode= */ false, /* addInitHeader= */ false);
            builder.AddReportHashId(requestId, seed, TReportCacheFlags(EReportCacheMode::Priemka));
            builder.Flush(request);

            UNIT_ASSERT(request.Cgi.Has("init_meta", "report_alice-cache-long-ttl=1"));
        }
    }

    Y_UNIT_TEST(ReportCacheCgi) {
        constexpr TStringBuf requestId = "ffffffff-ffff-ffff-ffff-ffffffffffff";
        constexpr TStringBuf seed = "qwerty";

        for (auto service : {EService::Megamind, EService::Bass}) {
            for (auto mode : {EReportCacheMode::Prod, EReportCacheMode::Priemka}) {
                {
                    TFakeRequestBuilder request;
                    TWebSearchBuilder builder(/* searchUi= */ "quasar", service, /* isChildMode= */ false, /* addInitHeader= */ false);
                    builder.AddReportHashId(requestId, seed, TReportCacheFlags(mode) | EReportCacheMode::DisableLookup);
                    builder.Flush(request);
                    UNIT_ASSERT(request.Cgi.Has("init_meta", "disable-report_alice-cache-lookup=1"));
                    UNIT_ASSERT(!request.Cgi.Has("init_meta", "disable-report_alice-cache-put=1"));
                }

                {
                    TFakeRequestBuilder request;
                    TWebSearchBuilder builder(/* searchUi= */ "quasar", service, /* isChildMode= */ false, /* addInitHeader= */ false);
                    builder.AddReportHashId(requestId, seed, TReportCacheFlags(mode) | EReportCacheMode::DisablePut);
                    builder.Flush(request);
                    UNIT_ASSERT(!request.Cgi.Has("init_meta", "disable-report_alice-cache-lookup=1"));
                    UNIT_ASSERT(request.Cgi.Has("init_meta", "disable-report_alice-cache-put=1"));
                }

                {
                    TFakeRequestBuilder request;
                    TWebSearchBuilder builder(/* searchUi= */ "quasar", service, /* isChildMode= */ false, /* addInitHeader= */ false);
                    builder.AddReportHashId(requestId, seed, TReportCacheFlags(mode) | EReportCacheMode::DisableLookup | EReportCacheMode::DisablePut);
                    builder.Flush(request);
                    UNIT_ASSERT(request.Cgi.Has("init_meta", "disable-report_alice-cache-lookup=1"));
                    UNIT_ASSERT(request.Cgi.Has("init_meta", "disable-report_alice-cache-put=1"));
                }

                {
                    TFakeRequestBuilder request;
                    TWebSearchBuilder builder(/* searchUi= */ "quasar", service, /* isChildMode= */ false, /* addInitHeader= */ false);
                    builder.AddReportHashId(requestId, seed, TReportCacheFlags(mode));
                    builder.Flush(request);
                    UNIT_ASSERT(!request.Cgi.Has("init_meta", "disable-report_alice-cache-lookup=1"));
                    UNIT_ASSERT(!request.Cgi.Has("init_meta", "disable-report_alice-cache-put=1"));
                }
            }

            {
                TFakeRequestBuilder request;
                TWebSearchBuilder builder(/* searchUi= */ "quasar", service, /* isChildMode= */ false, /* addInitHeader= */ false);
                builder.AddReportHashId(requestId, seed, EReportCacheMode::DisableLookup | EReportCacheMode::DisablePut);
                builder.Flush(request);
                UNIT_ASSERT(!request.Cgi.Has("init_meta", "disable-report_alice-cache-lookup=1"));
                UNIT_ASSERT(!request.Cgi.Has("init_meta", "disable-report_alice-cache-put=1"));
            }
        }
    }

    Y_UNIT_TEST(DisableAdsForNonSearch) {
        {
            TFakeRequestBuilder request;
            TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ false, /* addInitHeader= */ false);
            builder.Features().SetDisableAdsForNonSearch([]() { return true; });
            builder.GenericFixups();
            builder.Flush(request);
            UNIT_ASSERT(
                request.Cgi.Print().find("init_meta=disable-src-yabs&init_meta=disable-src-yabs_collection"
                                         "&init_meta=disable-src-yabs_distr&init_meta=disable-src-yabs_exp"
                                         "&init_meta=disable-src-yabs_gallery&init_meta=disable-src-yabs_gallery2"
                                         "&init_meta=disable-src-yabs_images&init_meta=disable-src-yabs_proxy"
                                         "&init_meta=disable-src-yabs_video") != TString::npos);
        }
        {
            TFakeRequestBuilder request;
            TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ false, /* addInitHeader= */ false);
            builder.Features().SetDisableAdsForNonSearch([]() { return false; });
            builder.GenericFixups();
            builder.Flush(request);
            UNIT_ASSERT(request.Cgi.Print().find("init_meta=disable-src-yabs") == TString::npos);
        }
    }

    Y_UNIT_TEST(DisableAdsInBass) {
        {
            TFakeRequestBuilder request;
            TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ false, /* addInitHeader= */ false);
            builder.Features().SetDisableAdsInBass([]() { return true; });
            builder.GenericFixups();
            builder.Flush(request);
            UNIT_ASSERT(
                request.Cgi.Print().find("init_meta=disable-src-yabs&init_meta=disable-src-yabs_collection"
                                         "&init_meta=disable-src-yabs_distr&init_meta=disable-src-yabs_exp"
                                         "&init_meta=disable-src-yabs_gallery&init_meta=disable-src-yabs_gallery2"
                                         "&init_meta=disable-src-yabs_images&init_meta=disable-src-yabs_proxy"
                                         "&init_meta=disable-src-yabs_video") != TString::npos);
        }
        {
            TFakeRequestBuilder request;
            TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ false, /* addInitHeader= */ false);
            builder.Features().SetDisableAdsInBass([]() { return false; });
            builder.GenericFixups();
            builder.Flush(request);
            UNIT_ASSERT(request.Cgi.Print().find("init_meta=disable-src-yabs") == TString::npos);
        }
    }

    Y_UNIT_TEST(DisableImages) {
        for (const auto explicitFlag : {true, false}) {
            for (const auto service : {EService::Bass, EService::Megamind}) {
                TFakeRequestBuilder request;
                TWebSearchBuilder builder(/* searchUi= */ "quasar", service, /* isChildMode= */ false, /* addInitHeader= */ false);
                if (explicitFlag) {
                    builder.Features().SetSkipImageSourcesCallback([]() { return true; });
                }
                builder.GenericFixups();
                builder.Flush(request);
                UNIT_ASSERT(request.Cgi.Has("srcskip", "IMAGESP"));
                UNIT_ASSERT(request.Cgi.Has("srcskip", "IMAGESQUICKP"));
                UNIT_ASSERT(request.Cgi.Has("srcskip", "IMAGESULTRAP"));
                UNIT_ASSERT(!request.Cgi.Has("exp_flags", "wizard_match_img_serp"));
            }
        }
    }

    Y_UNIT_TEST(EnableImages) {
        for (const auto service : {EService::Bass, EService::Megamind}) {
            TFakeRequestBuilder request;
            TWebSearchBuilder builder(/* searchUi= */ "quasar", service, /* isChildMode= */ false, /* addInitHeader= */ false);
            builder.Features().SetSkipImageSourcesCallback([]() { return false; });
            builder.GenericFixups();
            builder.Flush(request);
            UNIT_ASSERT(!request.Cgi.Has("srcskip", "IMAGESP"));
            UNIT_ASSERT(!request.Cgi.Has("srcskip", "IMAGESQUICKP"));
            UNIT_ASSERT(!request.Cgi.Has("srcskip", "IMAGESULTRAP"));
            UNIT_ASSERT(request.Cgi.Has("exp_flags", "wizard_match_img_serp"));
        }
    }

    Y_UNIT_TEST(DisableEverythingButPlatina) {
        {
            TFakeRequestBuilder request;
            TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ false, /* addInitHeader= */ false);

            builder.Flush(request);
            UNIT_ASSERT_VALUES_EQUAL(request.Cgi.NumOfValues("srcparams"), 0);

            builder.Features().SetDisableEverythingButPlatina([]() { return true; });
            builder.GenericFixups();
            builder.Flush(request);
            UNIT_ASSERT(request.Cgi.Has("srcparams", "WEB:srcskip=WEB,WEBFRESH_ON_MIDDLE,QUICK_SAMOHOD_ON_MIDDLE,QUICK_SAMOHOD_ON_MIDDLE_EXP,QUICK_SAMOHOD_ON_MIDDLE_EXP2,QUICK_SAMOHOD_ON_MIDDLE_EXP3"));
            UNIT_ASSERT(request.Cgi.Has("srcparams", "WEB_MISSPELL:srcskip=WEB,WEBFRESH_ON_MIDDLE,QUICK_SAMOHOD_ON_MIDDLE,QUICK_SAMOHOD_ON_MIDDLE_EXP,QUICK_SAMOHOD_ON_MIDDLE_EXP2,QUICK_SAMOHOD_ON_MIDDLE_EXP3"));
        }
        {
            TFakeRequestBuilder request;
            TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ false, /* addInitHeader= */ false);

            builder.Flush(request);
            UNIT_ASSERT_VALUES_EQUAL(request.Cgi.NumOfValues("srcparams"), 0);

            builder.Features().SetDisableEverythingButPlatina([]() { return true; });
            builder.GenericFixups();
            builder.Flush(request);
            UNIT_ASSERT(request.Cgi.Has("srcparams", "WEB:srcskip=WEB,WEBFRESH_ON_MIDDLE,QUICK_SAMOHOD_ON_MIDDLE,QUICK_SAMOHOD_ON_MIDDLE_EXP,QUICK_SAMOHOD_ON_MIDDLE_EXP2,QUICK_SAMOHOD_ON_MIDDLE_EXP3"));
            UNIT_ASSERT(request.Cgi.Has("srcparams", "WEB_MISSPELL:srcskip=WEB,WEBFRESH_ON_MIDDLE,QUICK_SAMOHOD_ON_MIDDLE,QUICK_SAMOHOD_ON_MIDDLE_EXP,QUICK_SAMOHOD_ON_MIDDLE_EXP2,QUICK_SAMOHOD_ON_MIDDLE_EXP3"));
        }
        {
            TFakeRequestBuilder request;
            TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ false, /* addInitHeader= */ false);

            builder.Flush(request);
            UNIT_ASSERT_VALUES_EQUAL(request.Cgi.NumOfValues("srcparams"), 0);

            builder.Features().SetDisableEverythingButPlatina([]() { return false; });
            builder.GenericFixups();
            builder.Flush(request);
            UNIT_ASSERT(!request.Cgi.Has("srcparams", "WEB:srcskip=WEB,WEBFRESH_ON_MIDDLE,QUICK_SAMOHOD_ON_MIDDLE,QUICK_SAMOHOD_ON_MIDDLE_EXP,QUICK_SAMOHOD_ON_MIDDLE_EXP2,QUICK_SAMOHOD_ON_MIDDLE_EXP3"));
            UNIT_ASSERT(!request.Cgi.Has("srcparams", "WEB_MISSPELL:srcskip=WEB,WEBFRESH_ON_MIDDLE,QUICK_SAMOHOD_ON_MIDDLE,QUICK_SAMOHOD_ON_MIDDLE_EXP,QUICK_SAMOHOD_ON_MIDDLE_EXP2,QUICK_SAMOHOD_ON_MIDDLE_EXP3"));
        }
    }

    Y_UNIT_TEST(CouldShowSerp) {
        {
            TFakeRequestBuilder request;
            TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ false, /* addInitHeader= */ false);
            builder.Features().SetCouldShowSerp([]() { return true; });
            builder.GenericFixups();
            builder.Flush(request);
            UNIT_ASSERT(request.Cgi.Has("rearr", "scheme_Local/Assistant/ClientCanShowSerp=1"));
        }
        {
            TFakeRequestBuilder request;
            TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ false, /* addInitHeader= */ false);
            builder.Features().SetCouldShowSerp([]() { return false; });
            builder.GenericFixups();
            builder.Flush(request);
            UNIT_ASSERT(request.Cgi.Has("rearr", "scheme_Local/Assistant/ClientCanShowSerp=0"));
        }
    }

    Y_UNIT_TEST(AddDirectCgi) {
        {
            TFakeRequestBuilder request;
            TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ false, /* addInitHeader= */ false);

            builder.Flush(request);
            UNIT_ASSERT_VALUES_EQUAL(request.Cgi.NumOfValues("srcparams"), 0);

            const TExpFlags emptyFlags;
            builder.AddDirectExperimentCgi(emptyFlags);
            builder.Flush(request);
            UNIT_ASSERT(request.Cgi.Has("direct_page", "620060"));
        }
        {
            TFakeRequestBuilder request;
            TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ false, /* addInitHeader= */ false);
            builder.Flush(request);
            UNIT_ASSERT_VALUES_EQUAL(request.Cgi.NumOfValues("srcparams"), 0);

            const TExpFlags flags{TRawExpFlags{{"direct_expid_for_search=1111", Nothing()}}};
            builder.AddDirectExperimentCgi(flags);
            builder.Flush(request);
            UNIT_ASSERT(request.Cgi.Has("direct_page", "620060"));
            UNIT_ASSERT(request.Cgi.Has("exp_flags", "direct_raw_parameters=experiment-id=1111"));
        }
    }

    Y_UNIT_TEST(AddCookies) {
        TFakeRequestBuilder request;
        UNIT_ASSERT(!request.Headers.HasHeader(NNetwork::HEADER_COOKIE));
        TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ false, /* addInitHeader= */ false);
        builder.AddCookies({"a=1", "b=2"}, /* uid= */ "12345");
        builder.Flush(request);
        const auto* header = request.Headers.FindHeader(NAlice::NNetwork::HEADER_COOKIE);
        UNIT_ASSERT(header);
        UNIT_ASSERT_VALUES_EQUAL(header->Value(),
                                 "i-m-not-a-hacker=ZJYnDvaNXYOmMgNiScSyQSGUDffwfSET; a=1; b=2; yandexuid=12345");
    }

    Y_UNIT_TEST(InternalFlagsBuilder_UpperSearchParams) {
        TFakeRequestBuilder request;
        TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ false, /* addInitHeader= */ false);
        builder.CreateInternalFlagsBuilder().AddUpperSearchParams("ru.yandex.quasar/1.0").Build();
        builder.Flush(request);
        const auto* header = request.Headers.FindHeader(NAlice::NNetwork::HEADER_X_YANDEX_INTERNAL_FLAGS);
        UNIT_ASSERT(header);
        UNIT_ASSERT_VALUES_EQUAL(
            header->Value(),
            Base64Encode(
                R"({"disable_redirects":1,"srcparams":{"UPPER":["rearr=scheme_Local/Facts/Create/EntityAsFactFlag=1","rearr=scheme_Local/Assistant/ClientId=\"ru.yandex.quasar/1.0\"","rearr=scheme_Local/Assistant/ClientIdBase64=\"cnUueWFuZGV4LnF1YXNhci8xLjA=\""]}})"));
    }

    Y_UNIT_TEST(InternalFlagsBuilder_DisableDirect) {
        TFakeRequestBuilder request;
        TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ false, /* addInitHeader= */ false);
        builder.CreateInternalFlagsBuilder().DisableDirect().Build();
        builder.Flush(request);
        const auto* header = request.Headers.FindHeader(NAlice::NNetwork::HEADER_X_YANDEX_INTERNAL_FLAGS);
        UNIT_ASSERT(header);
        UNIT_ASSERT_VALUES_EQUAL(header->Value(),
                                 Base64Encode(R"({"disable_redirects":1,"direct_raw_parameters":"aoff=1"})"));
    }

    Y_UNIT_TEST(EnableImageSources) {
        TFakeRequestBuilder request;
        TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ false, /* addInitHeader= */ false);
        builder.EnableImageSources();
        builder.Flush(request);
        UNIT_ASSERT(request.Cgi.Has("init_meta", "enable-images-in-alice"));
    }

    Y_UNIT_TEST(AddContentSettings) {
        {
            TFakeRequestBuilder request;
            TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ false, /* addInitHeader= */ false);
            builder.AddContentSettings();
            builder.Flush(request);
            UNIT_ASSERT(!request.Cgi.Has("fyandex"));
        }
        {
            TFakeRequestBuilder request;
            TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ true, /* addInitHeader= */ false);
            builder.AddContentSettings();
            builder.Flush(request);
            UNIT_ASSERT(request.Cgi.Has("fyandex", "1"));
            UNIT_ASSERT(!request.Cgi.Has("fyandex", "0"));
            UNIT_ASSERT(request.Cgi.Has("rearr", "scheme_Local/Facts/DirtyLanguage/CheckQuery=true"));
        }
        {
            TFakeRequestBuilder request;
            TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::BassMusic, /* isChildMode= */ true, /* addInitHeader= */ false);
            builder.AddContentSettings();
            builder.Flush(request);
            UNIT_ASSERT(!request.Cgi.Has("fyandex", "1"));
            UNIT_ASSERT(request.Cgi.Has("fyandex", "0"));
            UNIT_ASSERT(!request.Cgi.Has("rearr", "scheme_Local/Facts/DirtyLanguage/CheckQuery=true"));
        }
    }

    Y_UNIT_TEST(ShinyDiscovery) {
        TFakeRequestBuilder request;
        TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ false, /* addInitHeader= */ false);

        builder.SetupShinyDiscovery();

        builder.Flush(request);
        UNIT_ASSERT(request.Cgi.Has("rearr", "scheme_Local/ShinyDiscovery/SaasNamespace=shiny_discovery_metadoc_alice"));
        UNIT_ASSERT(request.Cgi.Has("rearr", "scheme_Local/ShinyDiscovery/InsertMethod=InsertPos"));
    }

    Y_UNIT_TEST(AddInitHeader) {
        {
            TFakeRequestBuilder request;
            TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ false, /* addInitHeader= */ false);
            builder.Flush(request);
            UNIT_ASSERT(!request.HasHeader(NNetwork::HEADER_X_YANDEX_REPORT_ALICE_REQUEST_INIT, "1"));
        }
        {
            TFakeRequestBuilder request;
            TWebSearchBuilder builder(/* searchUi= */ "quasar", EService::Megamind, /* isChildMode= */ false, /* addInitHeader= */ true);
            builder.Flush(request);
            UNIT_ASSERT(request.HasHeader(NNetwork::HEADER_X_YANDEX_REPORT_ALICE_REQUEST_INIT, "1"));
        }
    }

}

} // namespace
