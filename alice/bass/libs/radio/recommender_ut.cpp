#include "recommender.h"

#include <library/cpp/testing/unittest/registar.h>

using namespace NBASS::NRadio;

namespace {

TVector<size_t> RunAndCalculateDistribution(const TRadioRecommender& recommender,
                                            const TVector<TRadioRecommender::TRadioData>& radioDatas)
{
    constexpr int RUNS = 10000;
    TVector<size_t> distr(radioDatas.size());
    for (int i = 0; i < RUNS; ++i) {
        const auto& r = recommender.Recommend();
        size_t dist = std::distance(radioDatas.cbegin(), &r);
        ++distr[dist];
    }
    return distr;
}

} // anonymous namespace

Y_UNIT_TEST_SUITE(TRadioRecommenderTest) {
    Y_UNIT_TEST(EmptyDesiredAndModelTest) {
        NAlice::TRng rng(13071999);

        TVector<TRadioRecommender::TRadioData> radioDatas;
        radioDatas.push_back(TRadioRecommender::TRadioData{
            .RadioId = "fm_id1",
            .Title = "Первое Радио"
        });
        radioDatas.push_back(TRadioRecommender::TRadioData{
            .RadioId = "fm_id2",
            .Title = "Радио 2"
        });
        radioDatas.push_back(TRadioRecommender::TRadioData{
            .RadioId = "fm_id3",
            .Title = "НТВ Радио"
        });

        TRadioRecommender recommender(rng, radioDatas);

        const auto distr = RunAndCalculateDistribution(recommender, radioDatas);
        const TVector<size_t> expectedDistr{3319, 3375, 3306};
        UNIT_ASSERT(distr == expectedDistr);
    }

    Y_UNIT_TEST(DesiredRadiosTest) {
        NAlice::TRng rng(13071999);

        TVector<TRadioRecommender::TRadioData> radioDatas;
        // from best to worst
        for (int i = 1; i <= 7; ++i) {
            radioDatas.push_back(TRadioRecommender::TRadioData{
                .RadioId = TString::Join("fm_id", ToString(i)),
                .Title = TString::Join("id", ToString(i)),
            });
        }

        THashSet<TStringBuf> desiredRadioIds{"fm_id4", "fm_id2", "fm_id6", "fm_id_BR0K3N"};
        auto recommender = TRadioRecommender(rng, radioDatas).SetDesiredRadioIds(desiredRadioIds);

        const auto distr = RunAndCalculateDistribution(recommender, radioDatas);
        const TVector<size_t> expectedDistr{0, 10000, 0, 0, 0, 0, 0};
        UNIT_ASSERT(distr == expectedDistr);
    }

    // WARNING! this test is sensitive to the model resource
    Y_UNIT_TEST(WeightedMetatagModelTest) {
        NAlice::TRng rng(13071999);

        TVector<TRadioRecommender::TRadioData> radioDatas;
        radioDatas.push_back(TRadioRecommender::TRadioData{
            .RadioId = "fm_vostok_ulyanovsk", // "play_count": 300
            .Title = "Восток FM",
        });
        radioDatas.push_back(TRadioRecommender::TRadioData{
            .RadioId = "fm_dacha_barnaul", // "play_count": 106
            .Title = "РАДИО ДАЧА",
        });
        radioDatas.push_back(TRadioRecommender::TRadioData{
            .RadioId = "fm_kometa_chehov", // "play_count": 30
            .Title = "Радиостанция «КОМЕТА»",
        });
        radioDatas.push_back(TRadioRecommender::TRadioData{
            .RadioId = "fm_zvezda", // "play_count": 0
            .Title = "Радио ЗВЕЗДА",
        });

        auto recommender = TRadioRecommender(rng, radioDatas).UseMetatagWeightedModel("genre:uzbekpop");

        const auto distr = RunAndCalculateDistribution(recommender, radioDatas);
        const TVector<size_t> expectedDistr{6861, 2428, 711, 0}; // relative 300/106/30/0
        UNIT_ASSERT(distr == expectedDistr);
    }

    // WARNING! this test is sensitive to the model resource
    Y_UNIT_TEST(WeightedTrackIdModelTest) {
        NAlice::TRng rng(13071999);

        const TStringBuf trackId = "3905499"; // трек "Дорога сна"

        TVector<TRadioRecommender::TRadioData> radioDatas;
        radioDatas.push_back(TRadioRecommender::TRadioData{
            .RadioId = "fm_nashe_radio", // "play_count": 40
            .Title = "Наше радио",
        });
        radioDatas.push_back(TRadioRecommender::TRadioData{
            .RadioId = "fm_kpradio_habarovsk", // "play_count": 5
            .Title = "Комсомольская правда [Хабаровск]",
        });
        radioDatas.push_back(TRadioRecommender::TRadioData{
            .RadioId = "fm_nashe_barnaul", // "play_count": 30
            .Title = "Наше радио [Барнаул]",
        });
        radioDatas.push_back(TRadioRecommender::TRadioData{
            .RadioId = "fm_zvezda", // "play_count": 0
            .Title = "Радио ЗВЕЗДА",
        });

        auto recommender = TRadioRecommender(rng, radioDatas).UseTrackIdWeightedModel(trackId);

        const auto distr = RunAndCalculateDistribution(recommender, radioDatas);
        const TVector<size_t> expectedDistr{5359, 678, 3963, 0}; // relative 40/5/30/0
        UNIT_ASSERT(distr == expectedDistr);
    }
}
