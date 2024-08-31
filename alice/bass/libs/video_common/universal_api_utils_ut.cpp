#include "universal_api_utils.h"

#include <alice/bass/libs/video_common/video_ut_helpers.h>

#include <alice/library/unittest/fake_fetcher.h>
#include <alice/library/unittest/ut_helpers.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NBASS;
using namespace NHttpFetcher;
using namespace NTestingHelpers;
using namespace NVideoCommon;

namespace {

 // FIXME (a-sidorin@): anthology_movie type is not supported yet.
constexpr auto ROOT_ITEM_LIST = TStringBuf(R"({
    "content_items": [
      "tv_show:t1",
      "tv_show:t2",
      "tv_show:t3",
      "tv_show:t4",
      "anthology_movie:a1",
      "movie:m1",
      "movie:m2"
    ]
})");

constexpr auto MOVIE_1 = TStringBuf(R"({
    "content_item_id": "movie:m1",
    "content_type": "movie",
    "title": "Если только",
    "url_to_item_page": "https://www.kinopoisk.ru/film/23275",
    "duration": 5507,
    "genres": [ "фэнтези", "драма", "мелодрама", "комедия" ],
    "rating": 7.632,
    "release_year": 2004,
    "directors": [ "Джил Джангер" ],
    "actors": [ "Дженнифер Лав Хьюитт", "Пол Николлс", "Том Уилкинсон" ],
    "misc_ids": { "kinopoisk": "23275" },
    "children": [],
    "min_age": 12
})");

constexpr auto MOVIE_2 = TStringBuf(R"({
    "content_item_id": "movie:m2",
    "content_type": "movie",
    "title": "Три дня Виктора Чернышева",
    "duration": 5854,
    "genres": [ "драма" ],
    "rating": 7.26700019836,
    "release_year": 1967,
    "children": [],
    "min_age": 0
})");

constexpr auto TV_SHOW_OK = TStringBuf(R"({
    "content_item_id": "tv_show:t1",
    "content_type": "tv_show",
    "title": "Женщина в беде 3",
    "url_to_item_page": "https://www.kinopoisk.ru/film/961721",
    "genres": [ "детектив", "мелодрама" ],
    "rating": 5.764999866485596,
    "release_year": 2016,
    "directors": [ "Алексей Гусев" ],
    "actors": [ "Татьяна Казючиц", "Максим Щеголев", "Александр Клемантович" ],
    "misc_ids": { "kinopoisk": "961721" },
    "children": [
        "tv_show_season:t1_s1"
    ],
    "min_age": 12,
    "cover_url_2x3": "avatar2x3",
    "cover_url_16x9": "avatar16x9",
    "thumbnail_url_2x3_small": "avatar2x3_small",
    "thumbnail_url_16x9": "thumb16x9",
    "thumbnail_url_16x9_small": "thumb16x9_small"
})");

constexpr auto TV_SHOW_1_S1 = TStringBuf(R"({
    "content_item_id": "tv_show_season:t1_s1",
    "content_type": "tv_show_season",
    "title": "Женщина в беде 3",
    "description": "",
    "children": [
      "tv_show_episode:t1_s1_e1",
      "tv_show_episode:t1_s1_e2",
      "tv_show_episode:t1_s1_e3"
    ],
    "min_age": 12,
    "sequence_number": 1
})");

constexpr auto TV_SHOW_1_S1_E1 = TStringBuf(R"({
    "content_item_id": "tv_show_episode:t1_s1_e1",
    "content_type": "tv_show_episode",
    "title": "Женщина в беде 3 - Сезон 1 - Серия 1",
    "description": "",
    "duration": 2666,
    "children": [],
    "min_age": 12,
    "sequence_number": 1
})");

constexpr auto TV_SHOW_1_S1_E2 = TStringBuf(R"({
    "content_item_id": "tv_show_episode:t1_s1_e2",
    "content_type": "tv_show_episode",
    "title": "Женщина в беде 3 - Сезон 1 - Серия 2",
    "description": "",
    "duration": 2777,
    "children": [],
    "min_age": 12,
    "sequence_number": 2
})");

constexpr auto TV_SHOW_1_S1_E3 = TStringBuf(R"({
    "content_item_id": "tv_show_episode:t1_s1_e3",
    "content_type": "tv_show_episode",
    "title": "Женщина в беде 3 - Сезон 1 - Серия 3",
    "description": "",
    "duration": 2555,
    "children": [],
    "min_age": 12,
    "sequence_number": 3,
    "publication_date": "2019-02-12T00:00:00Z"
})");

constexpr auto TV_SHOW_MISSED_ITEMS = TStringBuf(R"({
     "content_item_id": "tv_show:t2",
     "content_type": "tv_show",
     "title": "Макс Грин и инопланетяне",
     "description": "",
     "url_to_item_page": "https://www.kinopoisk.ru/film/1048000",
     "genres": [ "мультфильм" ],
     "release_year": 2015,
     "directors": [ "Алексис Барросо" ],
     "actors": [ "Шеннон Конли" ],
     "misc_ids": {
       "kinopoisk": "1048000"
     },
     "children": [
       "tv_show_season:t2_s1",
       "tv_show_season:t2_s2"
     ],
     "min_age": 6
})");

constexpr auto TV_SHOW_2_S1 = TStringBuf(R"({
    "content_item_id": "tv_show_season:t2_s1",
    "content_type": "tv_show_season",
    "title": "Макс Грин и инопланетяне",
    "description": "",
    "release_year": 2016,
    "children": [
      "tv_show_episode:t2_s1_e1",
      "tv_show_episode:t2_s1_e2",
      "tv_show_episode:t2_s1_e3"
    ]
})");

constexpr auto TV_SHOW_2_S1_E1 = TStringBuf(R"({
    "content_item_id": "tv_show_episode:t2_s1_e1",
    "content_type": "tv_show_episode",
    "title": "Макс Грин и инопланетяне - Сезон 1 - Серия 1 - Пятница с закрытыми глазами",
    "duration": 670,
    "children": [],
    "min_age": 6,
    "sequence_number": 1,
    "publication_date": "2016-01-16T00:00:00Z",
})");

constexpr auto TV_SHOW_2_S1_E2 = TStringBuf(R"({
    "content_item_id": "tv_show_episode:t2_s1_e2",
    "content_type": "tv_show_episode",
    "title": "Макс Грин и инопланетяне - Сезон 1 - Серия 2 - Страх как он есть",
    "duration": 674,
    "children": [],
    "min_age": 6,
    "sequence_number": 2,
    "publication_date": "2016-01-16T00:00:00Z",
})");

constexpr auto TV_SHOW_BROKEN = TStringBuf(R"({
    "content_item_id": "tv_show:t3",
    "content_type": "tv_show",
    "title": "Боцман и попугай",
    "description": "О приключениях боцмана Ромы и его попугая",
    "url_to_item_page": "https://www.kinopoisk.ru/film/460133",
    "genres": [ "мультфильм" ],
    "misc_ids": {
      "kinopoisk": "460133"
    },
    "children": [],
    "min_age": 6
})");

constexpr auto TV_SHOW_OUT_OF_ORDER = TStringBuf(R"({
    "content_item_id": "tv_show:t4",
    "content_type": "tv_show",
    "title": "BBC: Галапагосы",
    "description": "Сериал про Галапагосские острова",
    "url_to_item_page": "https://okko.tv/mp_movie/galapagos2007",
    "genres": [ "Documentary" ],
    "misc_ids": {
      "kinopoisk": "308947",
      "imdb": null
     },
    "children": [
      "tv_show_episode:t4_e1",
      "tv_show_episode:t4_e2"
    ],
    "min_age": 6
})");

constexpr auto TV_SHOW_4_E1 = TStringBuf(R"({
    "content_item_id": "tv_show_episode:t4_e1",
    "content_type": "tv_show_episode",
    "title": "BBC: Галапагосы. Серия 1",
    "duration": 2940,
    "children": [],
    "min_age": 0,
    "sequence_number": 1
})");

constexpr auto TV_SHOW_4_E2 = TStringBuf(R"({
    "content_item_id": "tv_show_episode:t4_e2",
    "content_type": "tv_show_episode",
    "title": "BBC: Галапагосы. Серия 2",
    "duration": 2930,
    "children": [],
    "min_age": 0,
    "sequence_number": 2
})");

using TPageSet = THashMap<TStringBuf, TStringBuf>;

const TPageSet ITEM_SET = {
    {"all", ROOT_ITEM_LIST},
    {"movie:m1", MOVIE_1},
    {"movie:m2", MOVIE_2},
    {"tv_show:t1", TV_SHOW_OK},
    {"tv_show:t2", TV_SHOW_MISSED_ITEMS},
    {"tv_show:t3", TV_SHOW_BROKEN},
    {"tv_show:t4", TV_SHOW_OUT_OF_ORDER},
    {"tv_show_season:t1_s1", TV_SHOW_1_S1},
    {"tv_show_episode:t1_s1_e1", TV_SHOW_1_S1_E1},
    {"tv_show_episode:t1_s1_e2", TV_SHOW_1_S1_E2},
    {"tv_show_episode:t1_s1_e3", TV_SHOW_1_S1_E3},
    {"tv_show_season:t2_s1", TV_SHOW_2_S1},
    {"tv_show_episode:t2_s1_e1", TV_SHOW_2_S1_E1},
    {"tv_show_episode:t2_s1_e2", TV_SHOW_2_S1_E2},
    {"tv_show_episode:t4_e1", TV_SHOW_4_E1},
    {"tv_show_episode:t4_e2", TV_SHOW_4_E2},
};

const NSc::TValue EXPECTED_MOVIE_M1 = NSc::TValue::FromJson(R"({
    "actors": "Дженнифер Лав Хьюитт, Пол Николлс, Том Уилкинсон",
    "description": "",
    "directors": "Джил Джангер",
    "duration": 5507,
    "genre": "фэнтези, драма, мелодрама, комедия",
    "human_readable_id": "",
    "min_age": 12,
    "age_limit": "12",
    "misc_ids": {
      "kinopoisk": "23275"
    },
    "name": "Если только",
    "provider_item_id": "m1",
    "provider_name": "kinopoisk",
    "rating": 7.632,
    "release_year": 2004,
    "type": "movie",
    "debug_info": {
        "web_page_url": "http://www.kinopoisk.ru/film/23275"
    }
})");

const NSc::TValue EXPECTED_TVSHOW_T1 = NSc::TValue::FromJson(R"({
    "actors" : "Татьяна Казючиц, Максим Щеголев, Александр Клемантович",
    "cover_url_16x9" : "avatar16x9",
    "cover_url_2x3" : "avatar2x3",
    "description" : "",
    "directors" : "Алексей Гусев",
    "genre" : "детектив, мелодрама",
    "human_readable_id" : "",
    "min_age" : 12,
    "age_limit" : "12",
    "misc_ids" : {
        "kinopoisk" : "961721"
    },
    "name" : "Женщина в беде 3",
    "provider_item_id" : "t1",
    "provider_name" : "kinopoisk",
    "release_year" : 2016,
    "seasons_count" : 1,
    "thumbnail_url_16x9" : "thumb16x9",
    "thumbnail_url_16x9_small" : "thumb16x9_small",
    "thumbnail_url_2x3_small" : "avatar2x3_small",
    "type" : "tv_show",
    "debug_info": {
        "web_page_url": "http://www.kinopoisk.ru/film/961721"
    }
})");

const NSc::TValue EXPECTED_TVSHOW_T4 = NSc::TValue::FromJson(R"({
    "description" : "Сериал про Галапагосские острова",
    "genre" : "Documentary",
    "human_readable_id" : "",
    "min_age" : 6,
    "age_limit": "6",
    "misc_ids" : {
        "kinopoisk" : "308947"
    },
    "name" : "BBC: Галапагосы",
    "provider_item_id" : "t4",
    "provider_name" : "kinopoisk",
    "seasons_count" : 1,
    "type" : "tv_show",
    "debug_info": {
        "web_page_url": "http://www.kinopoisk.ru/film/308947"
    }
})");

const NSc::TValue EXPECTED_T1_S1_E1 = NSc::TValue::FromJson(R"({
    "description": "",
    "duration": 2666,
    "episode": 1,
    "human_readable_id": "",
    "min_age": 12,
    "age_limit": "12",
    "name": "1 серия",
    "provider_item_id": "t1_s1_e1",
    "provider_name": "kinopoisk",
    "provider_number": 1,
    "season" : 1,
    "seasons_count" : 1,
    "tv_show_item_id": "t1",
    "tv_show_season_id": "t1_s1",
    "type": "tv_show_episode"
})");

const NSc::TValue EXPECTED_T1_S1_E2 = NSc::TValue::FromJson(R"({
    "description": "",
    "duration": 2777,
    "episode": 2,
    "human_readable_id": "",
    "min_age": 12,
    "age_limit": "12",
    "name": "2 серия",
    "provider_item_id": "t1_s1_e2",
    "provider_name": "kinopoisk",
    "provider_number": 2,
    "season" : 1,
    "seasons_count" : 1,
    "tv_show_item_id": "t1",
    "tv_show_season_id": "t1_s1",
    "type": "tv_show_episode"
})");

const NSc::TValue EXPECTED_T1_S1_E3 = NSc::TValue::FromJson(R"({
    "description": "",
    "duration": 2555,
    "episode": 3,
    "human_readable_id": "",
    "min_age": 12,
    "age_limit": "12",
    "name": "3 серия",
    "provider_item_id": "t1_s1_e3",
    "provider_name": "kinopoisk",
    "provider_number": 3,
    "season" : 1,
    "seasons_count" : 1,
    "soon": 1,
    "tv_show_item_id": "t1",
    "tv_show_season_id": "t1_s1",
    "type": "tv_show_episode",
    "update_at_us": 1549929600000000
})");

const NSc::TValue EXPECTED_T4_E1 = NSc::TValue::FromJson(R"({
    "description" : "",
    "duration" : 2940,
    "episode" : 1,
    "human_readable_id" : "",
    "min_age" : 0,
    "age_limit" : "0",
    "name" : "BBC: Галапагосы. Серия 1",
    "provider_item_id" : "t4_e1",
    "provider_name" : "kinopoisk",
    "provider_number" : 1,
    "season" : 1,
    "seasons_count" : 1,
    "tv_show_item_id" : "t4",
    "type" : "tv_show_episode"
})");

const NSc::TValue EXPECTED_T4_E2 = NSc::TValue::FromJson(R"({
    "description" : "",
    "duration" : 2930,
    "episode" : 2,
    "human_readable_id" : "",
    "min_age" : 0,
    "age_limit" : "0",
    "name" : "BBC: Галапагосы. Серия 2",
    "provider_item_id" : "t4_e2",
    "provider_name" : "kinopoisk",
    "provider_number" : 2,
    "season" : 1,
    "seasons_count" : 1,
    "tv_show_item_id" : "t4",
    "type" : "tv_show_episode"
})");

THolder<TRequest> CreateFakePageSetRequest(const TPageSet& pageSet, const NUri::TUri& uri) {
    TString urlStr = uri.PrintS(NUri::TUri::EFlags::FlagPath);
    const TStringBuf itemId = TStringBuf(urlStr).RAfter('/');

    const auto* found = pageSet.FindPtr(itemId);
    return MakeHolder<NAlice::NTestingHelpers::TFakeRequest>(found ? TString{*found} : TString{});
}

class TFakePageSetMultiRequest : public IMultiRequest {
public:
    explicit TFakePageSetMultiRequest(const TPageSet& pageSet)
        : PageSet(pageSet)
    {
    }

    THolder<TRequest> AddRequest(const NUri::TUri& uri, const NHttpFetcher::TRequestOptions& /* options */) override {
        return CreateFakePageSetRequest(PageSet, uri);
    }

    void WaitAll(TInstant /* deadline */) override {
    }

private:
    const TPageSet& PageSet;
};

class TFakeRequestProvider : public IRequestProvider {
public:
    explicit TFakeRequestProvider(const TPageSet& pageSet)
        : PageSet(pageSet)
    {
    }

    TRequestPtr CreateSingleRequest(const NUri::TUri& uri, const TRequestOptions& /* options */) const override {
        return CreateFakePageSetRequest(PageSet, uri);
    }

    IMultiRequest::TRef CreateMultiRequest() const override {
        return new TFakePageSetMultiRequest(PageSet);
    }

private:
    const TPageSet& PageSet;
};

class TProviderSourceRequestFactory : public ISourceRequestFactory {
public:
    explicit TProviderSourceRequestFactory(IRequestProvider::TPtr requestProvider)
        : RequestProvider(requestProvider)
    {
    }

    // ISourceRequestFactory overrides:
    NHttpFetcher::TRequestPtr Request(TStringBuf path) override {
        return RequestProvider->CreateSingleRequest(Url(path), NHttpFetcher::TRequestOptions());
    }

    NHttpFetcher::TRequestPtr AttachRequest(TStringBuf path, NHttpFetcher::IMultiRequest::TRef multiRequest) override {
        return multiRequest->AddRequest(Url(path), NHttpFetcher::TRequestOptions());
    }

protected:
    virtual NUri::TUri Url(TStringBuf path) = 0;

private:
    IRequestProvider::TPtr RequestProvider;
};

class TDummySourceRequestFactory : public TProviderSourceRequestFactory {
public:
    explicit TDummySourceRequestFactory(IRequestProvider::TPtr requestProvider)
        : TProviderSourceRequestFactory(requestProvider)
    {
    }

protected:
    NUri::TUri Url(TStringBuf path) override {
        return NHttpFetcher::ParseUri(TString{"https://fake.tv/"} + path);
    }
};

void TestUpdateTime(TMaybe<TInstant> pageUpdateTime) {
    TStringBuilder rootItems;
    rootItems << TStringBuf(R"({ "content_items": [ "movie:m1" ])");
    if (pageUpdateTime)
        rootItems << R"(, "updated_at": )" << pageUpdateTime->ToStringUpToSeconds().Quote();
    rootItems << "}";

    const TPageSet rootOnly{{"all", rootItems}};

    auto fakeRequestProvider = MakeIntrusive<TFakeRequestProvider>(rootOnly);
    auto kpProvider = MakeIntrusive<TKinopoiskUAPIProvider>(MakeIntrusive<THttpUAPIRequestProvider>(
        std::make_unique<TDummySourceRequestFactory>(fakeRequestProvider)));
    auto infoProvider= MakeUAPIContentInfoProvider(fakeRequestProvider, kpProvider);
    TVideoItemList items;
    auto request = infoProvider->MakeContentListRequest(fakeRequestProvider->CreateMultiRequest());
    UNIT_ASSERT(!request->WaitAndParseResponse(items));

    UNIT_ASSERT_VALUES_EQUAL(items.Updated, pageUpdateTime);
    UNIT_ASSERT_VALUES_EQUAL(items.Items.size(), 1u);
}

Y_UNIT_TEST_SUITE(UAPIVideoProviderUnitTests) {
    auto fakeRequestProvider = MakeIntrusive<TFakeRequestProvider>(ITEM_SET);
    auto kpProvider = MakeIntrusive<TKinopoiskUAPIProvider>(MakeIntrusive<THttpUAPIRequestProvider>(
        std::make_unique<TDummySourceRequestFactory>(fakeRequestProvider)));

    Y_UNIT_TEST(ContentInfoProvider) {
        auto currentTime = TInstant::ParseIso8601("2018-02-15T00:00:00Z");
        auto infoProvider = MakeUAPIContentInfoProvider(fakeRequestProvider, kpProvider, 1u /* maxRPS */, currentTime);
        TVideoItem movie;
        TVideoItem tvShow;
        TVideoItem tvShowSingle;

        TSerialDescriptor tvShowSerial, tvShowSingleSerial;

        movie->ProviderItemId() = "m1";
        movie->Type() = ToString(EItemType::Movie);
        tvShow->ProviderItemId() = "t1";
        tvShowSingle->ProviderItemId() = "t4";
        tvShow->Type() = tvShowSingle->Type() = ToString(EItemType::TvShow);

        {
            UNIT_ASSERT(infoProvider->IsContentListAvailable());

            auto multiRequest = fakeRequestProvider->CreateMultiRequest();
            auto request = infoProvider->MakeContentListRequest(multiRequest);

            TVideoItemList contentList;
            const auto error = request->WaitAndParseResponse(contentList);
            UNIT_ASSERT(!error);

            TVector<TStringBuf> ids{ "m1", "m2", "t1", "t2", "t3", "t4" };
            UNIT_ASSERT_VALUES_EQUAL(contentList.Items.size(), ids.size());

            size_t movieCount = 0, tvShowCount = 0;
            for (size_t i = 0; i < ids.size(); ++i) {
                const TVideoItem& item = contentList.Items[i];
                UNIT_ASSERT(item->ProviderItemId() == ids[i]);
                size_t& count = item->Type() == "movie" ? movieCount : tvShowCount;
                ++count;
            }

            UNIT_ASSERT_VALUES_EQUAL(movieCount, 2);
            UNIT_ASSERT_VALUES_EQUAL(tvShowCount, 4);
        }

        {
            auto multiRequest = fakeRequestProvider->CreateMultiRequest();
            auto request = infoProvider->MakeContentInfoRequest(movie.Scheme(), multiRequest);
            const auto error = request->WaitAndParseResponse(movie);
            UNIT_ASSERT(!error);
            movie->GetRawValue()->Delete("provider_info");
            UNIT_ASSERT(EqualJson(EXPECTED_MOVIE_M1, *movie->GetRawValue()));
        }
        {
            auto multiRequest = fakeRequestProvider->CreateMultiRequest();
            auto request = infoProvider->MakeContentInfoRequest(tvShow.Scheme(), multiRequest);
            const auto error = request->WaitAndParseResponse(tvShow);
            UNIT_ASSERT(!error);
            tvShow->GetRawValue()->Delete("provider_info");
            tvShow->GetRawValue()->Delete("rating");
            UNIT_ASSERT(EqualJson(EXPECTED_TVSHOW_T1, *tvShow->GetRawValue()));
        }
        {
            auto multiRequest = fakeRequestProvider->CreateMultiRequest();
            auto request = infoProvider->MakeContentInfoRequest(tvShowSingle.Scheme(), multiRequest);
            const auto error = request->WaitAndParseResponse(tvShowSingle);
            UNIT_ASSERT(!error);
            tvShowSingle->GetRawValue()->Delete("provider_info");
            UNIT_ASSERT(EqualJson(EXPECTED_TVSHOW_T4, *tvShowSingle->GetRawValue()));
        }

        {
            auto multiRequest = fakeRequestProvider->CreateMultiRequest();
            auto request = infoProvider->MakeSerialDescriptorRequest(tvShow.Scheme(), multiRequest);
            const auto error = request->WaitAndParseResponse(tvShowSerial);
            UNIT_ASSERT(!error);
            UNIT_ASSERT_EQUAL(tvShowSerial.Id, TString{*tvShow->ProviderItemId()});
            UNIT_ASSERT_EQUAL(tvShowSerial.Seasons.size(), 1u);
            UNIT_ASSERT_EQUAL(tvShowSerial.Seasons[0].SerialId, tvShowSerial.Id);
            UNIT_ASSERT_EQUAL(tvShowSerial.Seasons[0].ProviderNumber, 1u);
        }
        {
            auto multiRequest = fakeRequestProvider->CreateMultiRequest();
            auto request = infoProvider->MakeSerialDescriptorRequest(tvShowSingle.Scheme(), multiRequest);
            const auto error = request->WaitAndParseResponse(tvShowSingleSerial);
            UNIT_ASSERT(!error);
            UNIT_ASSERT(tvShowSingleSerial.Id == TString{*tvShowSingle->ProviderItemId()});
            UNIT_ASSERT(tvShowSingleSerial.Seasons.size() == 1u);
            UNIT_ASSERT_EQUAL(tvShowSingleSerial.Seasons[0].Index, 0u);
            UNIT_ASSERT_EQUAL(tvShowSingleSerial.Seasons[0].ProviderNumber, 1u);
        }

        {
            auto multiRequest = fakeRequestProvider->CreateMultiRequest();
            auto request = infoProvider->MakeAllSeasonsDescriptorRequest(tvShowSerial, multiRequest);
            const auto& firstSeason = tvShowSerial.Seasons[0];
            auto& episodes = firstSeason.EpisodeItems;

            const auto error = request->WaitAndParseResponse(tvShowSerial);
            UNIT_ASSERT(!error);
            UNIT_ASSERT_EQUAL(episodes.size(), 3u);
            UNIT_ASSERT(EqualJson(EXPECTED_T1_S1_E1, *episodes[0]->GetRawValue()));
            UNIT_ASSERT(EqualJson(EXPECTED_T1_S1_E2, *episodes[1]->GetRawValue()));
            UNIT_ASSERT(EqualJson(EXPECTED_T1_S1_E3, *episodes[2]->GetRawValue()));
            UNIT_ASSERT_VALUES_EQUAL(firstSeason.Soon, false);
            UNIT_ASSERT_VALUES_EQUAL(firstSeason.UpdateAt, TInstant::ParseIso8601("2019-02-12T00:00:00Z"));
        }
        {
            auto multiRequest = fakeRequestProvider->CreateMultiRequest();
            auto request = infoProvider->MakeAllSeasonsDescriptorRequest(tvShowSingleSerial, multiRequest);
            auto& episodes = tvShowSingleSerial.Seasons[0].EpisodeItems;

            const auto error = request->WaitAndParseResponse(tvShowSingleSerial);
            UNIT_ASSERT(!error);
            UNIT_ASSERT(tvShowSingleSerial.Seasons[0].EpisodeItems.size() == 2u);
            UNIT_ASSERT(EqualJson(EXPECTED_T4_E1, *episodes[0]->GetRawValue()));
            UNIT_ASSERT(EqualJson(EXPECTED_T4_E2, *episodes[1]->GetRawValue()));
        }
    }

    Y_UNIT_TEST(GetOkkoIdData) {
        const NSc::TValue okkoMovie = NSc::TValue::FromJson(R"({
          "content_item_id": "movie:a098186c-1d35-4802-bf7d-c4dbec359e00",
          "content_type": "movie",
          "url_to_item_page": "https://okko.tv/movie/moonrise-kingdom",
          "release_year": 2012,
          "misc_ids": {
            "kinopoisk": "571892",
            "imdb": null
          },
          "min_age": 12,
          "sequence_number": 0
        })");
        const TString itemId = "movie:a098186c-1d35-4802-bf7d-c4dbec359e00";
        const TString providerItemId = "a098186c-1d35-4802-bf7d-c4dbec359e00";
        TOkkoUAPIParseHelper okkoParser;
        UNIT_ASSERT(okkoParser.GetProviderItemId(itemId));
        UNIT_ASSERT_STRINGS_EQUAL(*okkoParser.GetProviderItemId(itemId), providerItemId);
        UNIT_ASSERT_STRINGS_EQUAL(okkoParser.GetHumanReadableId(itemId, okkoMovie), "moonrise-kingdom");
    }

    Y_UNIT_TEST(UpdateTime) {
        TestUpdateTime(Nothing() /* pageUpdateTime */);
        TestUpdateTime(TInstant::Hours(1) /* pageUpdateTime */);
    }
}
} // namespace
