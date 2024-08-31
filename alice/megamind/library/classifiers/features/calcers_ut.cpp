#include "calcers.h"

#include <alice/megamind/library/factor_storage/factor_storage.h>
#include <alice/megamind/library/search/search.h>
#include <alice/megamind/library/testing/utils.h>

#include <alice/library/response_similarity/response_similarity.h>

#include <alice/megamind/protos/scenarios/features/music.pb.h>
#include <alice/megamind/protos/scenarios/features/search.pb.h>

#include <kernel/alice/search_scenario_factors_info/factors_gen.h>
#include <kernel/factor_storage/factor_storage.h>

#include <library/cpp/langs/langs.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/unittest/registar.h>


using namespace NAlice;
using namespace NAliceSearchScenario;

namespace {

bool CheckAllFactorsZero(const TFactorStorage& storage) {
    const auto view = storage.CreateConstView();

    // the comparison is chosen so that we detect NaN values
    return std::all_of(view.begin(), view.end(), [](const float factor) { return factor == 0.0f; });
}

Y_UNIT_TEST_SUITE(FillProtocolFactors) {
    Y_UNIT_TEST(Empty) {
        NMegamind::TScenarioFeatures features;
        TFactorStorage storage = CreateStorage();

        FillScenarioFactors(features, storage);
        UNIT_ASSERT(CheckAllFactorsZero(storage));
    }

    Y_UNIT_TEST(MusicFeatures) {
        NMegamind::TScenarioFeatures features;
        TFactorStorage storage = CreateStorage();

        auto& similarity = *features.MutableMusicFeatures()->MutableResult()->MutableTrackNameSimilarity();
        similarity = NResponseSimilarity::CalculateResponseItemSimilarity(TStringBuf("show me how to live"),
                                                                          TStringBuf("the show must go on"),
                                                                          ELanguage::LANG_RUS);

        UNIT_ASSERT(CheckAllFactorsZero(storage));
        FillScenarioFactors(features, storage);
        UNIT_ASSERT(!CheckAllFactorsZero(storage));
    }

    Y_UNIT_TEST(DirectFeatures) {
        NMegamind::TScenarioFeatures features;
        TFactorStorage storage = CreateStorage();

        auto& offerSimilarity = *features.MutableSearchFeatures()->MutableDirectOfferSimilarity();
        offerSimilarity = NResponseSimilarity::CalculateResponseItemSimilarity(TStringBuf("купить кирпичи в екатеринбурге"),
                                                                          TStringBuf("Мы продаем лучшие кирпичи!"),
                                                                          ELanguage::LANG_RUS);

        auto& titleSimilarity = *features.MutableSearchFeatures()->MutableDirectTitleSimilarity();
        titleSimilarity = NResponseSimilarity::CalculateResponseItemSimilarity(TStringBuf("купить кирпичи в екатеринбурге"),
                                                                          TStringBuf("Первая кирпичная компания"),
                                                                          ELanguage::LANG_RUS);

        auto& infoSimilarity = *features.MutableSearchFeatures()->MutableDirectInfoSimilarity();
        infoSimilarity = NResponseSimilarity::CalculateResponseItemSimilarity(TStringBuf("купить кирпичи в екатеринбурге"),
                                                                          TStringBuf("Екатеринбург, ул. Шаумяна, 73"),
                                                                          ELanguage::LANG_RUS);

        UNIT_ASSERT(CheckAllFactorsZero(storage));
        FillScenarioFactors(features, storage);
        UNIT_ASSERT(!CheckAllFactorsZero(storage));
    }
}

Y_UNIT_TEST_SUITE(FillSearchFactors) {
    Y_UNIT_TEST(FullResponse) {
        TFactorStorage storage = CreateStorage();
        const auto response = TSearchResponse(NResource::Find("search_response.json"), "reqid", TRTLogger::StderrLogger(), /* initDataSources= */ true);
        FillSearchFactors("какие новости", response, storage);
        UNIT_ASSERT(!CheckAllFactorsZero(storage));

        const TFactorView view = storage.CreateViewFor(NFactorSlices::EFactorSlice::ALICE_SEARCH_SCENARIO);

        UNIT_ASSERT_EQUAL(view[FI_MUSIC_WIZARD_POS_DCG], 0);
        UNIT_ASSERT_EQUAL(view[FI_LYRICS_WIZARD_POS_DCG], 0);
        UNIT_ASSERT_GT(view[FI_VIDEO_WIZARD_POS_DCG], 0.1);
        UNIT_ASSERT_EQUAL(view[FI_IMAGE_WIZARD_POS_DCG], 0);
        UNIT_ASSERT_EQUAL(view[FI_TRANSLATE_WIZARD_POS_DCG], 0);
        UNIT_ASSERT_EQUAL(view[FI_WEATHER_WIZARD_POS_DCG], 0);
        UNIT_ASSERT_EQUAL(view[FI_CONVERTER_WIZARD_POS_DCG], 0);
        UNIT_ASSERT_EQUAL(view[FI_MAPS_WIZARD_POS_DCG], 0);
        UNIT_ASSERT_EQUAL(view[FI_ROUTES_WIZARD_POS_DCG], 0);
        UNIT_ASSERT_EQUAL(view[FI_NEWS_WIZARD_POS_DCG], 0);
        UNIT_ASSERT_EQUAL(view[FI_MARKET_WIZARD_POS_DCG], 0);
        UNIT_ASSERT_EQUAL(view[FI_GEOV_WIZARD_POS_DCG], 0);

        UNIT_ASSERT_EQUAL(view[FI_MUSIC_HOSTS_COUNT], 0);
        UNIT_ASSERT_EQUAL(view[FI_VIDEO_HOSTS_COUNT], 0);
        UNIT_ASSERT_EQUAL(view[FI_MUSIC_HOSTS_DCG_AT_1], 0);
        UNIT_ASSERT_EQUAL(view[FI_VIDEO_HOSTS_DCG_AT_1], 0);
        UNIT_ASSERT_EQUAL(view[FI_MUSIC_HOSTS_DCG_AT_5], 0);
        UNIT_ASSERT_EQUAL(view[FI_VIDEO_HOSTS_DCG_AT_5], 0);
        UNIT_ASSERT_EQUAL(view[FI_MUSIC_HOSTS_DCG], 0);
        UNIT_ASSERT_EQUAL(view[FI_VIDEO_HOSTS_DCG], 0);

        UNIT_ASSERT_GT(view[FI_QUERY_IN_TITLE_RATIO_MAX], 0.4);
        UNIT_ASSERT_GT(view[FI_QUERY_IN_TITLE_RATIO_MEAN], 0.3);
        UNIT_ASSERT_GT(view[FI_QUERY_IN_TITLE_RATIO_MIN], 0.1);

        UNIT_ASSERT_GT(view[FI_TITLE_IN_QUERY_RATIO_MAX], 0.3);
        UNIT_ASSERT_GT(view[FI_TITLE_IN_QUERY_RATIO_MEAN], 0.2);
        UNIT_ASSERT_GT(view[FI_TITLE_IN_QUERY_RATIO_MIN], 0.1);

        UNIT_ASSERT_GT(view[FI_QUERY_IN_PREFIX_RATIO_MAX], 0.5);
        UNIT_ASSERT_GT(view[FI_QUERY_IN_PREFIX_RATIO_MEAN], 0.4);
        UNIT_ASSERT_EQUAL(view[FI_QUERY_IN_PREFIX_RATIO_MIN], 0.0);

        UNIT_ASSERT_GT(view[FI_QUERY_IN_DOUBLE_PREFIX_RATIO_MAX], 0.4);
        UNIT_ASSERT_GT(view[FI_QUERY_IN_DOUBLE_PREFIX_RATIO_MEAN], 0.3);
        UNIT_ASSERT_EQUAL(view[FI_QUERY_IN_DOUBLE_PREFIX_RATIO_MIN], 0.0);

        UNIT_ASSERT_GT(view[FI_QUERY_IN_HEADLINE_RATIO_MAX], 0.3);
        UNIT_ASSERT_GT(view[FI_QUERY_IN_HEADLINE_RATIO_MEAN], 0.2);
        UNIT_ASSERT_EQUAL(view[FI_QUERY_IN_HEADLINE_RATIO_MIN], 0.0);

        UNIT_ASSERT_GT(view[FI_HEADLINE_IN_QUERY_RATIO_MAX], 0.0);
        UNIT_ASSERT_GT(view[FI_HEADLINE_IN_QUERY_RATIO_MEAN], 0.0);
        UNIT_ASSERT_EQUAL(view[FI_HEADLINE_IN_QUERY_RATIO_MIN], 0.0);

        UNIT_ASSERT_GT(view[FI_QUERY_IN_HEADLINE_PREFIX_RATIO_MAX], 0.3);
        UNIT_ASSERT_GT(view[FI_QUERY_IN_HEADLINE_PREFIX_RATIO_MEAN], 0.1);
        UNIT_ASSERT_EQUAL(view[FI_QUERY_IN_HEADLINE_PREFIX_RATIO_MIN], 0.0);

        UNIT_ASSERT_GT(view[FI_QUERY_IN_DOUBLE_HEADLINE_PREFIX_RATIO_MAX], 0.1);
        UNIT_ASSERT_GT(view[FI_QUERY_IN_DOUBLE_HEADLINE_PREFIX_RATIO_MEAN], 0.0);
        UNIT_ASSERT_EQUAL(view[FI_QUERY_IN_DOUBLE_HEADLINE_PREFIX_RATIO_MIN], 0.0);
    }
}

} // namespace
