#include "clicks_computer.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/vector.h>

using namespace NAlice::NMusicWebSearch::NListeningCounters;

TVector<TUserListening> CreateTestData() {
    TString puid = "1234";

    TUserListening first;
    first.SetPuid(puid);
    first.SetReqId("1");
    first.SetTimespent(0.0F);
    first.SetTimestamp(1630516975);
    first.SetUrl("https://music.yandex.ru/track/14705409");

    TUserListening second;
    second.SetPuid(puid);
    second.SetReqId("2");
    second.SetTimespent(4.8127);
    second.SetTimestamp(1630516976);
    second.SetUrl("https://music.yandex.ru/track/26158681");

    TUserListening third;
    third.SetPuid(puid);
    third.SetReqId("3");
    third.SetTimespent(0.0897);
    third.SetTimestamp(1630516977);
    third.SetUrl("https://music.yandex.ru/users/yakidsaudio/playlists/1043/");

    return {first, second, third};
}


Y_UNIT_TEST_SUITE(GeneratePossibleUrls) {
    Y_UNIT_TEST(Artist) {
        TStringBuf url = "https://music.yandex.ru/artist/155917/tracks?sort=title&dir=asc";
        THashSet<TString> expected = {
            "https://music.yandex.ru/artist/155917/tracks?sort=title&dir=asc",
            "https://music.yandex.ru/artist/155917/tracks",
            "https://music.yandex.ru/artist/155917",
            "https://music.yandex.ru/artist/155917/albums",
        };

        UNIT_ASSERT_EQUAL(expected, NImpl::GeneratePossibleUrls(url));
    }

    Y_UNIT_TEST(Track) {
        TStringBuf url = "https://music.yandex.ru/album/3582130/track/29733783?mob=0&play=1";
        THashSet<TString> expected = {
            "https://music.yandex.ru/album/3582130/track/29733783?mob=0&play=1",
            "https://music.yandex.ru/album/3582130/track/29733783",
            "https://music.yandex.ru/track/29733783",
        };

        UNIT_ASSERT_EQUAL(expected, NImpl::GeneratePossibleUrls(url));
    }

    Y_UNIT_TEST(Album) {
        TStringBuf url = "https://music.yandex.ru/album/12050893";
        THashSet<TString> expected = {
            "https://music.yandex.ru/album/12050893",
        };

        UNIT_ASSERT_EQUAL(expected, NImpl::GeneratePossibleUrls(url));
    }

    Y_UNIT_TEST(Playlist) {
        TStringBuf url = "https://music.yandex.ru/users/music-blog/playlists/2427?playTrack=28701510";
        THashSet<TString> expected = {
            "https://music.yandex.ru/users/music-blog/playlists/2427?playTrack=28701510",
            "https://music.yandex.ru/users/music-blog/playlists/2427",
            "https://music.yandex.ru/users/music-blog/playlists/2427?rich-tracks=false",
        };

        UNIT_ASSERT_EQUAL(expected, NImpl::GeneratePossibleUrls(url));
    }
}


Y_UNIT_TEST_SUITE(SanityChecks) {
    Y_UNIT_TEST(ComputeCounters) {
        auto config = NImpl::CreateConfig();
        auto data = CreateTestData();

        auto counters = NImpl::ComputeCounters(config, data.begin(), data.end());

        UNIT_ASSERT(counters.Errors.empty());
    }

    Y_UNIT_TEST(AggregateCounters) {
        auto config = NImpl::CreateConfig();
        auto data = CreateTestData();

        auto counters = NImpl::ComputeCounters(config, data.begin(), data.end());
        UNIT_ASSERT(counters.Errors.empty());

        auto aggregation = NImpl::AggregateCounters(config, counters);
        UNIT_ASSERT(aggregation.Errors.empty());
    }
}
