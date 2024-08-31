#include "kinopoisk_recommendations.h"
#include "defs.h"

#include <alice/bass/libs/video_common/kinopoisk_recommendations.h>
#include <alice/bass/ut/helpers.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/vector.h>

#include <utility>

using namespace NBASS::NVideo;
using namespace NVideoCommon;

namespace {
using TData = TKinopoiskRecommendations::TData;
using TInfo = TKinopoiskFilmInfo;

NAlice::TRng rng(42);

const auto COMEDY_REQUEST = NSc::TValue::FromJson(R"({
  "form": {
    "name": "personal_assistant.scenarios.video_recommendation",
    "slots": [
      {"name": "film_genre", "optional": true, "type": "custom.video_film_genre", "value": "comedy"}
    ]
  },
  "meta": {
    "epoch": 1578676778,
    "tz": "Europe/Moscow",
    "utterance": "посоветуй комедии"
  }
}
)");

const auto YEAR_REQUEST = NSc::TValue::FromJson(R"({
  "form": {
    "name": "personal_assistant.scenarios.video_recommendation",
    "slots": [
      {"name": "release_date", "optional": true, "type": "date", "value": "2015"}
    ]
  },
  "meta": {
    "epoch": 1578676778,
    "tz": "Europe/Moscow",
    "utterance": "посоветуй фильмы 2015 года"
  }
}
)");

const auto DRAMA_REQUEST = NSc::TValue::FromJson(R"({
  "form": {
    "name": "personal_assistant.scenarios.video_recommendation",
    "slots": [
      {"name": "film_genre", "optional": true, "type": "custom.video_film_genre", "value": "drama"}
    ]
  },
  "meta": {
    "epoch": 1578676778,
    "tz": "Europe/Moscow",
    "utterance": "посоветуй драмы"
  }
}
)");

TVector<TInfo> RecommendTop(const TData& data, size_t limit, const TFlags<EContentType>& contentType) {
    TVector<TInfo> top;
    data.RecommendTop(limit, contentType, Nothing() /* genre */, [&top](const TInfo& info) { top.push_back(info); });
    return top;
}

TVector<TInfo> RecommendRandom(const TData& data, size_t limit, const TFlags<EContentType>& contentType) {
    TVector<TInfo> top;
    data.RecommendRandom(limit, contentType, Nothing() /* genre */,
                         [&top](const TInfo& info) { top.push_back(info); }, rng);
    return top;
}

TVector<TInfo> RecommendMultistep(const TData& data, size_t limit,
                                  const TFlags<EContentType>& contentType, const TVideoSlots& slots) {
    TVector<TInfo> top;
    data.RecommendMultistep(limit, contentType, slots,
                            [&top](const TInfo& info) { top.push_back(info); });
    return top;
}

bool Contains(const TVector<TInfo>& infos, const TInfo& info) {
    for (const auto& any : infos) {
        if (any.AlmostEqual(info))
            return true;
    }
    return false;
}

TMaybe<TVideoSlots> GetSlots(const NSc::TValue& request) {
    const auto ctx = NTestingHelpers::MakeContext(request);
    return TVideoSlots::TryGetFromContext(*ctx);
}

Y_UNIT_TEST_SUITE(KinopoiskRecommendationsTests) {
    Y_UNIT_TEST(Smoke) {
        TVector<TInfo> all;

        TInfo mashaAndTheBear;
        mashaAndTheBear.Id = "masha-i-medved";
        mashaAndTheBear.Rating = 7.8;
        mashaAndTheBear.ContentType = EContentType::Cartoon;
        mashaAndTheBear.ReleaseDate = NDatetime::TCivilDay{2007};
        mashaAndTheBear.Genres = {EVideoGenre::Comedy, EVideoGenre::Family};
        all.push_back(mashaAndTheBear);

        TInfo fixiki;
        fixiki.Id = "fixiki";
        fixiki.Rating = 7.5;
        fixiki.ContentType = EContentType::Cartoon;
        fixiki.ReleaseDate = NDatetime::TCivilDay{2010};
        fixiki.Genres = {EVideoGenre::Childrens};
        all.push_back(fixiki);

        TInfo triKota;
        triKota.Id = "tri-kota";
        triKota.Rating = 7.4;
        triKota.ContentType = EContentType::Cartoon;
        triKota.ReleaseDate = NDatetime::TCivilDay{2015};
        triKota.Genres = {EVideoGenre::Comedy, EVideoGenre::Childrens};
        all.push_back(triKota);

        TData data(all);

        for (const auto& recommend : {&RecommendTop, &RecommendRandom}) {
            {
                const auto top = recommend(data, 0 /* limit */, ~TFlags<EContentType>{});
                UNIT_ASSERT(top.empty());
            }

            {
                const auto top = recommend(data, 10 /* limit */, EContentType::Movie);
                UNIT_ASSERT(top.empty());
            }

            {
                const auto top = recommend(data, 1000 /* limit */, ~TFlags<EContentType>{});
                UNIT_ASSERT_VALUES_EQUAL(top.size(), 3);
                UNIT_ASSERT(top[0].AlmostEqual(mashaAndTheBear));
                UNIT_ASSERT(top[1].AlmostEqual(fixiki));
                UNIT_ASSERT(top[2].AlmostEqual(triKota));
            }
        }

        {
            const auto top = RecommendTop(data, 2 /* limit */, ~TFlags<EContentType>{});
            UNIT_ASSERT_VALUES_EQUAL(top.size(), 2);
            UNIT_ASSERT(top[0].AlmostEqual(mashaAndTheBear));
            UNIT_ASSERT(top[1].AlmostEqual(fixiki));
        }

        {
            const auto top = RecommendRandom(data, 2 /* limit */, ~TFlags<EContentType>{});
            UNIT_ASSERT_VALUES_EQUAL(top.size(), 2);

            THashSet<TString> ids;
            TMaybe<double> prevRating;

            for (const auto& info : top) {
                // Checks that returned ids are unique.
                UNIT_ASSERT(ids.insert(info.Id).second);

                // Checks that returned infos are from the list of all
                // infos.
                UNIT_ASSERT(Contains(all, info));

                // Checks that returned infos are ordered descending
                // by rating.
                if (prevRating)
                    UNIT_ASSERT(info.Rating <= *prevRating);
                prevRating = info.Rating;
            }
        }

        {
            const TVector<NSc::TValue> requests = {
                COMEDY_REQUEST,
                YEAR_REQUEST,
                DRAMA_REQUEST
            };

            const TVector<TVector<TString>> expectedIdsList = {
                {mashaAndTheBear.Id, triKota.Id},
                {triKota.Id},
                {}
            };

            for (size_t testId = 0; testId < requests.size(); ++testId) {
                const TMaybe<TVideoSlots> slots = GetSlots(requests[testId]);
                UNIT_ASSERT(slots);

                const auto expectedIds = expectedIdsList[testId];

                const auto top = RecommendMultistep(data, 3 /* limit */, ~TFlags<EContentType>{}, *slots);
                UNIT_ASSERT_VALUES_EQUAL(top.size(), expectedIds.size());

                for (size_t i = 0; i < top.size(); ++i) {
                    UNIT_ASSERT_VALUES_EQUAL(top[i].Id, expectedIds[i]);
                }
            }
        }
    }
}
} // namespace
