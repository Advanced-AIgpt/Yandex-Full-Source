#include "recommender.h"

#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood {

namespace {

constexpr TStringBuf DATA_PATH = "alice/hollywood/library/scenarios/suggesters/movies/ut/data/ut";

constexpr TStringBuf UP = "Вверх";
constexpr TStringBuf UP_SUGGESTION = "Хотите посмотреть мультфильм «Вверх»? "
                                           "Чудесная история о дружбе, любви и мечтах.";
const TString UP_ID = "451faf4d11fb7fc5b2e1dc964b6e49fb";

constexpr TStringBuf GUMP = "Форрест Гамп";
constexpr TStringBuf GUMP_SUGGESTION = "Хотите посмотреть «Форреста Гампа»? Говорят, это один из тех случаев, "
                                             "когда фильм оказался в разы лучше книги. Ну что, включаю?";
const TString GUMP_ID = "46a0706f3a37eb52b933f510ae5c7d21";

constexpr TStringBuf WOLF = "Волк с Уолл-стрит";
constexpr TStringBuf WOLF_SUGGESTION = "Хотите посмотреть фильм «Волк с Уолл-стрит»? "
                                             "Один из эпизодов с Леонардо ДиКаприо помог фильму "
                                             "выиграть премию за самый безумный эпизод.";
const TString WOLF_ID = "4d05a2c799d34ac9bb644af70c8e5dc1";

} // namespace

Y_UNIT_TEST_SUITE(TRecommenderSuite) {
    Y_UNIT_TEST(TestRecommender) {
        TMovieRecommender recommender;
        recommender.LoadFromPath(BinaryPath(DATA_PATH));

        NAlice::TFakeRng rng;

        {
            TMovieRecommender::TRestrictions restrictions;
            restrictions.Age = EContentRestrictionLevel::Children;

            const auto* item = recommender.Recommend(restrictions, rng);

            UNIT_ASSERT_VALUES_EQUAL(item->Texts[0], UP_SUGGESTION);
            UNIT_ASSERT_VALUES_EQUAL(item->Name, UP);
        }
        {
            TMovieRecommender::TRestrictions restrictions;
            restrictions.Age = EContentRestrictionLevel::Medium;

            const auto* item = recommender.Recommend(restrictions, rng);

            UNIT_ASSERT_VALUES_EQUAL(item->Texts[0], GUMP_SUGGESTION);
            UNIT_ASSERT_VALUES_EQUAL(item->Name, GUMP);
        }
        {
            TMovieRecommender::TRestrictions restrictions;
            restrictions.Age = EContentRestrictionLevel::Without;

            const auto* item = recommender.Recommend(restrictions, rng);

            UNIT_ASSERT_VALUES_EQUAL(item->Texts[0], WOLF_SUGGESTION);
            UNIT_ASSERT_VALUES_EQUAL(item->Name, WOLF);
        }
        {
            TMovieRecommender::TRestrictions restrictions;
            restrictions.Age = EContentRestrictionLevel::Medium;
            restrictions.ItemIds = {WOLF_ID};

            const auto* item = recommender.Recommend(restrictions, rng);

            UNIT_ASSERT_VALUES_EQUAL(item->Texts[0], GUMP_SUGGESTION);
            UNIT_ASSERT_VALUES_EQUAL(item->Name, GUMP);
        }
        {
            TMovieRecommender::TRestrictions restrictions;
            restrictions.Age = EContentRestrictionLevel::Without;
            restrictions.ItemIds = {WOLF_ID, GUMP_ID};

            const auto* item = recommender.Recommend(restrictions, rng);

            UNIT_ASSERT_VALUES_EQUAL(item->Texts[0], UP_SUGGESTION);
            UNIT_ASSERT_VALUES_EQUAL(item->Name, UP);
        }
        {
            TMovieRecommender::TRestrictions restrictions;
            restrictions.Age = EContentRestrictionLevel::Without,
            restrictions.ItemIds = {WOLF_ID, GUMP_ID, UP_ID};

            const auto* item = recommender.Recommend(restrictions, rng);

            UNIT_ASSERT(item == nullptr);
        }
        {
            TMovieRecommender::TRestrictions restrictions;
            restrictions.Age = EContentRestrictionLevel::Without;
            restrictions.Genre = "adventure";

            const auto* item = recommender.Recommend(restrictions, rng);

            UNIT_ASSERT_VALUES_EQUAL(item->Texts[0], UP_SUGGESTION);
            UNIT_ASSERT_VALUES_EQUAL(item->Name, UP);
        }
        {
            TMovieRecommender::TRestrictions restrictions;
            restrictions.Age = EContentRestrictionLevel::Without;
            restrictions.ContentType = "cartoon";

            const auto* item = recommender.Recommend(restrictions, rng);

            UNIT_ASSERT_VALUES_EQUAL(item->Texts[0], UP_SUGGESTION);
            UNIT_ASSERT_VALUES_EQUAL(item->Name, UP);
        }
        {
            TMovieRecommender::TRestrictions restrictions;
            restrictions.Age = EContentRestrictionLevel::Without;
            restrictions.ContentType = "cartoon";
            restrictions.Genre = "detective";

            const auto* item = recommender.Recommend(restrictions, rng);

            UNIT_ASSERT(item == nullptr);
        }
    }
}

} // namespace NAlice::NHollywood
