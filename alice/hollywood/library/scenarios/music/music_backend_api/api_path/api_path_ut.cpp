#include "api_path.h"

#include <alice/library/json/json.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood::NMusic {

Y_UNIT_TEST_SUITE(ApiPathTest) {

Y_UNIT_TEST(Artist) {
    NApiPath::TApiPathRequestParams requestParams(/* pageIdx = */ 3, /* pageSize = */ 30);
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::TrackOfArtist("654321", requestParams, "007"),
                              "/artists/654321/tracks?__uid=007&page=3&pageSize=30");
}

Y_UNIT_TEST(ArtistBriefInfo) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::ArtistBriefInfo(/* artistId = */ "123456", /* userId = */ "007"),
                              "/artists/123456/brief-info?__uid=007");
}

Y_UNIT_TEST(ArtistTracks) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::ArtistTracks(/* artistId = */ "123456", /* userId = */ "007"),
                              "/artists/123456/tracks?__uid=007");
}

Y_UNIT_TEST(Album) {
    NApiPath::TApiPathRequestParams requestParams(/* pageIdx = */ 3, /* pageSize = */ 30);
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::AlbumTracks("112233", requestParams, true, "007", false),
                              "/albums/112233/with-tracks?__uid=007&page=3&pageSize=30&richTracks=true");
}

Y_UNIT_TEST(LatestAlbumOfArtist) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::LatestAlbumOfArtist("112233", "007"),
                              "/artists/112233/direct-albums?page=0&page-size=1&sort-by=year&sort-order=desc&__uid=007");
}

Y_UNIT_TEST(SingleTrack) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::SingleTrack("456789", "007"), "/tracks/456789?__uid=007");
}

Y_UNIT_TEST(DownloadInfoMp3GetAlice) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::DownloadInfoMp3GetAlice("123123123", "007"),
                              "/tracks/123123123/download-info?isAliceRequester=true&__uid=007");
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::DownloadInfoMp3GetAlice("123123123", "007", DownloadInfoFlag::HQ),
                              "/tracks/123123123/download-info?isAliceRequester=true&__uid=007&formatFlags=hq");
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::DownloadInfoMp3GetAlice("123123123", "007", DownloadInfoFlag::LQ),
                              "/tracks/123123123/download-info?isAliceRequester=true&__uid=007&formatFlags=lq");
}

Y_UNIT_TEST(PlayAudioPlaysUid) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::PlayAudioPlays("", "007"),
                              "/plays?__uid=007");
}

Y_UNIT_TEST(PlayAudioPlaysClientNowUid) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::PlayAudioPlays("1600000000", "007"),
                              "/plays?__uid=007&client-now=1600000000");
}

Y_UNIT_TEST(LikeAlbum) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::LikeAlbum("007", "112233"),
                              "/users/007/likes/albums/add?album-id=112233&__uid=007");
}

Y_UNIT_TEST(LikeArtist) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::LikeArtist("007", "112233"),
                              "/users/007/likes/artists/add?artist-id=112233&__uid=007");
}

Y_UNIT_TEST(DislikeArtist) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::DislikeArtist("007", "112233"),
                              "/users/007/dislikes/artists/add?artist-id=112233&__uid=007");
}

Y_UNIT_TEST(LikeGenre) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::LikeGenre("007", "pop"),
                              "/users/007/likes/genres/add?genre=pop&__uid=007");
}

Y_UNIT_TEST(LikeTrack) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::LikeTrack("007", "456", "123"),
                              "/users/007/likes/tracks/add?track-id=123:456&__uid=007");
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::LikeTrack("007", "456", "123", "radio-session-id", "batch-id"),
                              "/users/007/likes/tracks/add?track-id=123:456&__uid=007&radioSessionId=radio-session-id&batchId=batch-id");
}

Y_UNIT_TEST(RemoveLikeTrack) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::RemoveLikeTrack("007", "456", "123"),
                              "/users/007/likes/tracks/123:456/remove?__uid=007");
}

Y_UNIT_TEST(LikesTracks) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::LikesTracks("007"),
                              "/users/007/likes/tracks?__uid=007");
}

Y_UNIT_TEST(DislikesTracks) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::DislikesTracks("007"),
                              "/users/007/dislikes/tracks?__uid=007");
}

Y_UNIT_TEST(DislikeTrack) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::DislikeTrack("007", "456", "123"),
                              "/users/007/dislikes/tracks/add?track-id=123:456&__uid=007");
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::DislikeTrack("007", "456", "123", "radio-session-id", "batch-id"),
                              "/users/007/dislikes/tracks/add?track-id=123:456&__uid=007&radioSessionId=radio-session-id&batchId=batch-id");
}

Y_UNIT_TEST(RemoveDislikeTrack) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::RemoveDislikeTrack("007", "456", "123"),
                              "/users/007/dislikes/tracks/123:456/remove?__uid=007");
}

Y_UNIT_TEST(PlaylistSearch) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::PlaylistSearch("007", "helloworld"),
                              "/search?type=playlist&page=0&text=helloworld&__uid=007");
}

Y_UNIT_TEST(SpecialPlaylistOfTheDay) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::SpecialPlaylist("007", "playlistOfTheDay"),
                              "/playlists/personal/playlistOfTheDay?__uid=007");
}

Y_UNIT_TEST(PlaylistTracksForMusicSdk) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::PlaylistTracksForMusicSdk("007", "105590476", "1250"),
                              "/users/105590476/playlists/1250?rich-tracks=false&withSimilarsLikesCount=true&__uid=007");
}

Y_UNIT_TEST(RadioFeedbackNoBatchId) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::RadioFeedback("type:tag"),
                              "/station/type:tag/feedback");
}

Y_UNIT_TEST(RadioFeedback) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::RadioFeedback("type:tag", "1234-1234-123456-1234"),
                              "/station/type:tag/feedback?batch-id=1234-1234-123456-1234");
}

Y_UNIT_TEST(RadioFeedbackWithRadioSessionId) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::RadioFeedback("type:tag", "1234-1234-123456-1234", "foobarbaz"),
                              "/station/type:tag/feedback?batch-id=1234-1234-123456-1234&radio-session-id=foobarbaz");
}

Y_UNIT_TEST(RadioTracksEmptyQueue) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::RadioTracks("type:tag", /* queue = */ {}, /* newRadioSession = */ false),
                              "/station/type:tag/tracks");
}

Y_UNIT_TEST(RadioTracks) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::RadioTracks("type:tag", /* queue = */ {"123abc", "45de", "fg67890"}
                              , /* newRadioSession = */ false),
                              "/station/type:tag/tracks?queue=123abc%2C45de%2Cfg67890");
}

Y_UNIT_TEST(RadioSessionFeedback) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::RadioSessionFeedback("a650ca26-ada4-4c62-a842-7d6c646d534c"),
                              "/session/a650ca26-ada4-4c62-a842-7d6c646d534c/feedback");
}

Y_UNIT_TEST(RadioTracksNewRadioSession) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::RadioTracks("type:tag", /* queue = */ {"123abc", "45de", "fg67890"},
                                                    /* newRadioSession = */ true),
                              "/station/type:tag/tracks?new-radio-session=true&queue=123abc%2C45de%2Cfg67890");
}

Y_UNIT_TEST(GenerativeStream) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::GenerativeStream("generative:focus"), "/station/generative:focus/stream");
}

Y_UNIT_TEST(GenerativeFeedback) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::GenerativeFeedback("generative:lucky", "123"), "/station/generative:lucky/feedback?streamId=123");
}

Y_UNIT_TEST(InfiniteFeed) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::InfiniteFeed(/* userId = */ "007"),
                              "/infinite-feed?landingType=navigator&supportedBlocks=generic&__uid=007");
}

Y_UNIT_TEST(ChildrenLandingCatalogue) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::ChildrenLandingCatalogue(/* userId = */ "007"),
                              "/children-landing/catalogue?requestedBlocks=CATEGORIES_TAB&__uid=007");
}

Y_UNIT_TEST(AfterTrack) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::AfterTrack("007", "hollywood", "context", "contextItem", "123", "321"),
                              "/after-track?__uid=007&context=context&contextItem=contextItem&from=hollywood&nextTrackId=321&prevTrackId=123&types=shot");
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::AfterTrack("007", "hollywood", "context", "contextItem", "123", {}),
                              "/after-track?__uid=007&context=context&contextItem=contextItem&from=hollywood&prevTrackId=123&types=shot");
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::AfterTrack("007", "hollywood", "context", "contextItem", {}, "321"),
                              "/after-track?__uid=007&context=context&contextItem=contextItem&from=hollywood&nextTrackId=321&types=shot");
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::AfterTrack("007", "hollywood", {}, "contextItem", "123", "321"),
                              "/after-track?__uid=007&from=hollywood&nextTrackId=321&prevTrackId=123&types=shot");
}

Y_UNIT_TEST(GenreOverview) {
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::GenreOverview("rusestrada", /* artistsCount= */ 15, /* userId = */ "007"),
                              "/genre-overview?__uid=007&albums-count=0&artists-count=15&genre=rusestrada&promotions-count=0&tracks-count=0");
}

Y_UNIT_TEST(ConstructMetaPathFromCoverUri) {
    const TStringBuf coverUri = "https://avatars.yandex.net/get-music-content/95061/942b9259.a.7166032-1/200x200";
    const TStringBuf metaPath = "/getinfo-music-content/95061/942b9259.a.7166032-1/meta";
    UNIT_ASSERT_STRINGS_EQUAL(NApiPath::ConstructMetaPathFromCoverUri(coverUri), metaPath);
}

Y_UNIT_TEST(RadioNewSessionBody) {
    // check without "queue"
    NJson::TJsonValue expected1;
    expected1["seeds"].AppendValue("genre:rock");
    expected1["seeds"].AppendValue("mood:happy");
    expected1["includeTracksInResponse"] = true;
    expected1["incognito"] = true;
    expected1["child"] = true;
    expected1["allowExplicit"] = false;
    expected1["aliceExperiments"]["force_ichwill"] = "1";
    expected1["aliceExperiments"]["exp1"] = "val1";
    expected1["aliceExperiments"]["exp2"] = "val2";
    expected1["trackToStartFrom"] = "123";
    expected1["clientRemoteType"] = "pult";

    auto [path1, body1] = NApiPath::RadioNewSessionPathAndBody(
        /* radioStationIds = */ {"genre:rock", "mood:happy"},
        /* queue = */ {},
        /* biometryData = */ {.IsIncognitoUser = true, .IsChild = true},
        /* filtrationMode = */ NScenarios::TUserPreferences_EFiltrationMode_Safe,
        /* flags = */ {{"exp1", "val1"}, {"exp2", "val2"}},
        /* startFromTrackId = */ "123",
        /* fromSpecified = */ true,
        /* useIchwill = */ true
    );
    UNIT_ASSERT_VALUES_EQUAL(path1, "/session/new");
    UNIT_ASSERT_VALUES_EQUAL(NAlice::JsonFromString(body1), expected1);

    // check with "queue"
    NJson::TJsonValue expected2;
    expected2["seeds"].AppendValue("genre:classicalmasterpieces");
    expected2["seeds"].AppendValue("epoch:eighteens");
    expected2["seeds"].AppendValue("mood:sad");
    expected2["queue"].AppendValue("01234");
    expected2["queue"].AppendValue("0123-0123");
    expected2["includeTracksInResponse"] = true;
    expected2["incognito"] = false;
    expected2["child"] = false;
    expected2["allowExplicit"] = true;
    expected2["clientRemoteType"] = "alice";

    auto [path2, body2] = NApiPath::RadioNewSessionPathAndBody(
        /* radioStationIds = */ {"genre:classicalmasterpieces", "epoch:eighteens", "mood:sad"},
        /* queue = */ {"01234", "0123-0123"},
        /* biometryData = */ {.IsIncognitoUser = false, .IsChild = false},
        /* filtrationMode = */ NScenarios::TUserPreferences_EFiltrationMode_NoFilter,
        /* flags = */ {},
        /* startFromTrackId = */ Default<TString>(),
        /* fromSpecified = */ false,
        /* useIchwill = */ false
    );
    UNIT_ASSERT_VALUES_EQUAL(path2, "/session/new");
    UNIT_ASSERT_VALUES_EQUAL(NAlice::JsonFromString(body2), expected2);
}

Y_UNIT_TEST(RadioSessionTracksBody) {
    // check with "queue"
    const TVector<TStringBuf> queue{"01234", "0123-0123"};

    NJson::TJsonValue expected;
    expected["queue"].AppendValue("01234");
    expected["queue"].AppendValue("0123-0123");
    expected["aliceExperiments"]["flag1"] = "val1";

    auto [path1, body1] = NApiPath::RadioSessionTracksPathAndBody("a650ca26-ada4-4c62-a842-7d6c646d534c", queue,
                                                                  /* flags = */ {{"flag1", "val1"}}, /* useIchwill = */ false);
    UNIT_ASSERT_VALUES_EQUAL(path1, "/session/a650ca26-ada4-4c62-a842-7d6c646d534c/tracks");
    UNIT_ASSERT_VALUES_EQUAL(NAlice::JsonFromString(body1), expected);

    // check empty body
    auto [path2, body2] = NApiPath::RadioSessionTracksPathAndBody("a650ca26-ada4-4c62-a842-7d6c646d534c", /* queue = */ {},
                                                                  /* flags = */ {}, /* useIchwill = */ false);
    UNIT_ASSERT_VALUES_EQUAL(path2, "/session/a650ca26-ada4-4c62-a842-7d6c646d534c/tracks");
    UNIT_ASSERT_VALUES_EQUAL(body2, "{}");
}

};

}  // namespace NAlice::NHollywood::NMusic
