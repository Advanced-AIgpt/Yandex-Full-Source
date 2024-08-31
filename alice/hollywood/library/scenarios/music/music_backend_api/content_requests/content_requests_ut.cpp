#include "content_requests.h"

#include <apphost/lib/service_testing/service_testing.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood::NMusic {

Y_UNIT_TEST_SUITE(ContentRequestsTest) {

Y_UNIT_TEST(Artist) {
    NScenarios::TScenarioBaseRequest baseReq;
    NAppHost::NService::TTestContext serviceCtx;
    TScenarioBaseRequestWrapper request{baseReq, serviceCtx};

    TMusicContext mCtx;
    auto scState = *mCtx.MutableScenarioState();

    auto& pg = *scState.MutableQueue()->MutableNextContentLoadingState()->MutablePaged();
    pg.SetPageIdx(2);

    auto& pb = *scState.MutableQueue()->MutablePlaybackContext();
    pb.MutableContentId()->SetType(TContentId_EContentType_Artist);
    pb.MutableContentId()->SetId("123123");

    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    mq.SetConfig(CreateMusicConfig({}));
    NScenarios::TRequestMeta meta;
    const auto result = PrepareContentRequest(request, mq, mCtx, MakeAtomicShared<TRequestMetaProvider>(meta), TRTLogger::NullLogger(),
                                              /* biometryData = */ {.UserId = "007"},
                                              /* requesterUserId = */ {"008"});
    UNIT_ASSERT_EQUAL(result.GetPath(), "/artists/123123/tracks?__uid=008&page=2&pageSize=20");
}

Y_UNIT_TEST(Album) {
    NScenarios::TScenarioBaseRequest baseReq;
    NAppHost::NService::TTestContext serviceCtx;
    TScenarioBaseRequestWrapper request{baseReq, serviceCtx};

    TMusicContext mCtx;
    auto scState = *mCtx.MutableScenarioState();

    auto& pg = *scState.MutableQueue()->MutableNextContentLoadingState()->MutablePaged();
    pg.SetPageIdx(2);

    auto& pb = *scState.MutableQueue()->MutablePlaybackContext();
    pb.MutableContentId()->SetType(TContentId_EContentType_Album);
    pb.MutableContentId()->SetId("321321");

    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    mq.SetConfig(CreateMusicConfig({}));
    NScenarios::TRequestMeta meta;
    const auto result = PrepareContentRequest(request, mq, mCtx, MakeAtomicShared<TRequestMetaProvider>(meta), TRTLogger::NullLogger(),
                                              /* biometryData = */ {.UserId = "007"},
                                              /* requesterUserId = */ {"008"});
    UNIT_ASSERT_EQUAL(result.GetPath(), "/albums/321321/with-tracks?__uid=008&page=2&pageSize=20&richTracks=true");
}

Y_UNIT_TEST(Playlist) {
    NScenarios::TScenarioBaseRequest baseReq;
    NAppHost::NService::TTestContext serviceCtx;
    TScenarioBaseRequestWrapper request{baseReq, serviceCtx};

    TMusicContext mCtx;
    auto scState = *mCtx.MutableScenarioState();

    auto& pg = *scState.MutableQueue()->MutableNextContentLoadingState()->MutablePaged();
    pg.SetPageIdx(2);

    auto& pb = *scState.MutableQueue()->MutablePlaybackContext();
    pb.MutableContentId()->SetType(TContentId_EContentType_Playlist);
    pb.MutableContentId()->SetId("33445566:66554433");

    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    mq.SetConfig(CreateMusicConfig({}));
    NScenarios::TRequestMeta meta;
    const auto result = PrepareContentRequest(request, mq, mCtx, MakeAtomicShared<TRequestMetaProvider>(meta), TRTLogger::NullLogger(),
                                              /* biometryData = */ {.UserId = "007"},
                                              /* requesterUserId = */ {"008"});
    UNIT_ASSERT_EQUAL(result.GetPath(), "/users/33445566/playlists/66554433?__uid=008&page=2&pageSize=20&rich-tracks=true");
}

Y_UNIT_TEST(SingleTrack) {
    NScenarios::TScenarioBaseRequest baseReq;
    NAppHost::NService::TTestContext serviceCtx;
    TScenarioBaseRequestWrapper request{baseReq, serviceCtx};

    TMusicContext mCtx;
    auto scState = *mCtx.MutableScenarioState();

    TExpFlags expFlags;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    mq.SetConfig(CreateMusicConfig({}));

    auto& pb = *scState.MutableQueue()->MutablePlaybackContext();
    pb.MutableContentId()->SetType(TContentId_EContentType_Track);
    pb.MutableContentId()->SetId("321321");
    NScenarios::TRequestMeta meta;
    const auto result = PrepareContentRequest(request, mq, mCtx, MakeAtomicShared<TRequestMetaProvider>(meta), TRTLogger::NullLogger(),
                                              /* biometryData = */ {.UserId = "007"},
                                              /* requesterUserId = */ {"008"});
    UNIT_ASSERT_EQUAL(result.GetPath(), "/tracks/321321?__uid=008");
}

Y_UNIT_TEST(RadioNewSession) {
    NScenarios::TScenarioBaseRequest baseReq;
    NAppHost::NService::TTestContext serviceCtx;
    auto& exps = *baseReq.MutableExperiments()->mutable_fields();
    exps["exp1"].set_string_value("val1");
    exps["exp2"].set_string_value("val2");
    baseReq.MutableUserPreferences()->SetFiltrationMode(NScenarios::TUserPreferences_EFiltrationMode_Safe);
    TScenarioBaseRequestWrapper request{baseReq, serviceCtx};

    NScenarios::TRequestMeta meta;
    meta.SetOAuthToken("XXXX-xxxx");

    TMusicContext mCtx;
    auto scState = *mCtx.MutableScenarioState();
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    mq.SetConfig(CreateMusicConfig({}));

    scState.MutableQueue()->AddHistory()->SetTrackId("links-2-3-4");
    auto& ci = *scState.MutableQueue()->MutablePlaybackContext()->MutableContentId();
    ci.SetType(TContentId_EContentType_Radio);
    ci.SetId("genre:rock");
    ci.AddIds("genre:rock");
    ci.AddIds("mood:happy");

    mCtx.SetNeedRadioSkip(true);
    mCtx.SetFirstPlay(true);
    const auto result = PrepareContentRequest(request, mq, mCtx, MakeAtomicShared<TRequestMetaProvider>(meta), TRTLogger::NullLogger(),
            /* biometryData = */ {.IsIncognitoUser = false, .IsChild = false, .UserId = "007"},
            /* requesterUserId = */ {"008"});
    UNIT_ASSERT_EQUAL(result.GetPath(), "/session/new");
    UNIT_ASSERT_EQUAL(result.GetMethod(), NAppHostHttp::THttpRequest_EMethod_Post);

    const auto* authHeader = FindIfPtr(result.GetHeaders(), [](const auto& header) { return header.GetName() == "authorization"; });
    UNIT_ASSERT(authHeader);
    UNIT_ASSERT_EQUAL(authHeader->GetValue(), "OAuth XXXX-xxxx");

    NJson::TJsonValue expectedContent;
    expectedContent["seeds"].AppendValue("genre:rock");
    expectedContent["seeds"].AppendValue("mood:happy");
    expectedContent["queue"].AppendValue("links-2-3-4");
    expectedContent["aliceExperiments"]["exp1"] = "val1";
    expectedContent["aliceExperiments"]["exp2"] = "val2";
    expectedContent["includeTracksInResponse"] = true;
    expectedContent["incognito"] = false;
    expectedContent["allowExplicit"] = false;
    expectedContent["child"] = true;
    expectedContent["clientRemoteType"] = "alice";
    UNIT_ASSERT_VALUES_EQUAL(NAlice::JsonFromString(result.GetContent()), expectedContent);
}

Y_UNIT_TEST(RadioSessionTrack) {
    NScenarios::TScenarioBaseRequest baseReq;
    NAppHost::NService::TTestContext serviceCtx;
    auto& exps = *baseReq.MutableExperiments()->mutable_fields();
    exps["exp1"].set_string_value("val1");
    exps["exp2"].set_string_value("val2");
    baseReq.MutableUserPreferences()->SetFiltrationMode(NScenarios::TUserPreferences_EFiltrationMode_Safe);
    TScenarioBaseRequestWrapper request{baseReq, serviceCtx};

    NScenarios::TRequestMeta meta;
    meta.SetOAuthToken("XXXX-xxxx");
    const TClientInfoProto clientInfo;

    TMusicContext mCtx;
    auto scState = *mCtx.MutableScenarioState();
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    mq.SetConfig(CreateMusicConfig({}));

    scState.MutableQueue()->AddHistory()->SetTrackId("links-2-3-4");
    scState.MutableQueue()->MutablePlaybackContext()->MutableContentId()->SetType(TContentId_EContentType_Radio);

    auto& radio = *scState.MutableQueue()->MutableCurrentContentLoadingState()->MutableRadio();
    radio.SetSessionId("012-345");
    radio.SetBatchId("bababa-12345");

    mCtx.SetNeedRadioSkip(true);
    mCtx.SetFirstPlay(false);
    const auto result = PrepareContentRequest(request, mq, mCtx, MakeAtomicShared<TRequestMetaProvider>(meta), TRTLogger::NullLogger(),
            /* biometryData = */ {.IsIncognitoUser = false, .IsChild = false, .UserId = "007"},
            /* requesterUserId = */ {"008"});
    UNIT_ASSERT_EQUAL(result.GetPath(), "/session/012-345/tracks");
    UNIT_ASSERT_EQUAL(result.GetMethod(), NAppHostHttp::THttpRequest_EMethod_Post);

    const auto* authHeader = FindIfPtr(result.GetHeaders(), [](const auto& header) { return header.GetName() == "authorization"; });
    UNIT_ASSERT(authHeader);
    UNIT_ASSERT_EQUAL(authHeader->GetValue(), "OAuth XXXX-xxxx");

    NJson::TJsonValue expectedContent;
    expectedContent["queue"].AppendValue("links-2-3-4");
    expectedContent["aliceExperiments"]["exp1"] = "val1";
    expectedContent["aliceExperiments"]["exp2"] = "val2";
    UNIT_ASSERT_VALUES_EQUAL(NAlice::JsonFromString(result.GetContent()), expectedContent);
}

}

} //namespace NAlice::NHollywood::NMusic
