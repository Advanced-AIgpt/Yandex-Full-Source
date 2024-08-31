#include "entity_searcher.h"
#include "entity_searcher_builder.h"
#include "sample_graph.h"

#include <alice/nlu/granet/lib/sample/entity.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>


using namespace NAlice::NNlu;

Y_UNIT_TEST_SUITE(EntitySearcher) {
    Y_UNIT_TEST(TestEntitySeacher) {
        TVector<TEntityString> entityStrings = {
            {
                .Sample = "здравствуйте",
                .Type = "hello",
                .Value = "hello",
                .LogProbability = -1.5
            },
            {
                .Sample = "до свидания",
                .Type = "goodbye",
                .Value = "goodbye"
            },
            {
                .Sample = "как дела",
                .Type = "how_r_u",
                .Value = "how_r_u"
            },
            {
                .Sample = "здравствуй алиса как дело",
                .Type = "hello_alice_how_r_u",
                .Value = "hello_alice_how_r_u",
                .LogProbability = -10.
            }
        };
        const double LEMMA_LOG_PROB = -1.;
        TEntitySearcher entitySearcher(TEntitySearcherDataBuilder().Build(entityStrings));
        TVector<NGranet::TEntity> synonyms = {
            NGranet::TEntity{
                .Interval = {0, 1},
                .Type = "syn.thesaurus_lemma",
                .Value = "здравствуй,здорова,здравствуйте",
                .LogProbability = -2.
            },
            NGranet::TEntity{
                .Interval = {2, 4},
                .Type = "syn.translit_ru",
                .Value = "kak dela",
                .LogProbability = -3.
            }
        };
        TVector<NGranet::TEntity> searcherResult = entitySearcher.Search(TSampleGraph::FromGranetSample([&synonyms] {
            NGranet::TSample::TRef sample = NGranet::TSample::Create("привет алиса как дела", LANG_RUS);
            sample->AddEntitiesOnTokens(synonyms);
            return sample;
        }(), /* addSynonyms= */ true));
        const TVector<NGranet::TEntity> ExpectedFoundEntities = {
            NGranet::TEntity{
                .Interval = {0, 1},
                .Type = "hello",
                .Value = "hello",
                .LogProbability = synonyms[0].LogProbability + LEMMA_LOG_PROB +
                                  entityStrings[0].LogProbability
            },
            NGranet::TEntity{
                .Interval = {0, 4},
                .Type = "hello_alice_how_r_u",
                .Value = "hello_alice_how_r_u",
                .LogProbability = entityStrings[3].LogProbability + synonyms[0].LogProbability +
                                  LEMMA_LOG_PROB + LEMMA_LOG_PROB
            },
            NGranet::TEntity{
                .Interval = {2, 4},
                .Type = "how_r_u",
                .Value = "how_r_u",
                .LogProbability = 0.
            }
        };
        UNIT_ASSERT_VALUES_EQUAL(searcherResult, ExpectedFoundEntities);
    }

    Y_UNIT_TEST(TestEntitySeacherNominativesAsLemma) {
        TVector<TEntityString> entityStrings = {
            {
                .Sample = "любимая",
                .Type = "my_love",
                .Value = "my_love",
            },
            {
                .Sample = "любимые",
                .Type = "my_loves",
                .Value = "my_loves",
            }
        };
        const double LEMMA_LOG_PROB = -1.;
        TEntitySearcher entitySearcher(TEntitySearcherDataBuilder().Build(entityStrings));
        TVector<NGranet::TEntity> searcherResult = entitySearcher.Search(TSampleGraph::FromGranetSample([] {
            NGranet::TSample::TRef sample = NGranet::TSample::Create("позвони любимой или любимым", LANG_RUS);
            return sample;
        }(), /* addSynonyms= */ true, /* addNominativesAsLemma= */ true));
        const TVector<NGranet::TEntity> ExpectedFoundEntities = {
            NGranet::TEntity{
                .Interval = {1, 2},
                .Type = "my_love",
                .Value = "my_love",
                .LogProbability = LEMMA_LOG_PROB + entityStrings[0].LogProbability
            },
            NGranet::TEntity{
                .Interval = {3, 4},
                .Type = "my_loves",
                .Value = "my_loves",
                .LogProbability = LEMMA_LOG_PROB + entityStrings[1].LogProbability
            },
        };
        UNIT_ASSERT_VALUES_EQUAL(searcherResult, ExpectedFoundEntities);
    }
}
