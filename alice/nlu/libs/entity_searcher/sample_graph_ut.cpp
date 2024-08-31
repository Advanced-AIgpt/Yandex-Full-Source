#include "sample_graph.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/vector.h>

using namespace NAlice::NNlu;

Y_UNIT_TEST_SUITE(SampleGraph) {
    Y_UNIT_TEST(TestSampleGraph) {
        const double LEMMA_LOG_PROB = -1.;
        const TVector<NGranet::TEntity> synonyms = {
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
        const TSampleGraph graph = TSampleGraph::FromGranetSample([&synonyms]{
            NGranet::TSample::TRef sample = NGranet::TSample::Create("привет алиса как дела", LANG_RUS);
            sample->AddEntitiesOnTokens(synonyms);
            return sample;
        }(), /* addSynonyms= */ false);
        const TVector<TVector<TSampleGraph::TArc>> expectedArcs = {
            {
                {1, "привет", 0.}
            },
            {
                {2, "алиса", 0.}
            },
            {
                {3, "как", 0.},
            },
            {
                {4, "дела", 0},
                {4, "дело", LEMMA_LOG_PROB}
            }
        };
        for (size_t i = 0; i < expectedArcs.size(); ++i) {
            UNIT_ASSERT_EQUAL(graph.GetArcsOnVertex(i), expectedArcs[i]);
        }
    }

    Y_UNIT_TEST(TestSampleGraphSynonyms) {
        const double LEMMA_LOG_PROB = -1.;
        const TVector<NGranet::TEntity> synonyms = {
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
        const TSampleGraph graph = TSampleGraph::FromGranetSample([&synonyms]{
            NGranet::TSample::TRef sample = NGranet::TSample::Create("привет алиса как дела", LANG_RUS);
            sample->AddEntitiesOnTokens(synonyms);
            return sample;
        }(), /* addSynonyms= */ true);
        const TVector<TVector<TSampleGraph::TArc>> expectedArcs = {
            {
                {1, "здорова", synonyms[0].LogProbability + LEMMA_LOG_PROB},
                {1, "здравствуй", synonyms[0].LogProbability + LEMMA_LOG_PROB},
                {1, "здравствуйте", synonyms[0].LogProbability + LEMMA_LOG_PROB},
                {1, "привет", 0.}
            },
            {
                {2, "алиса", 0.}
            },
            {
                {3, "как", 0.},
                {4, "kak dela", synonyms[1].LogProbability}
            },
            {
                {4, "дела", 0},
                {4, "дело", LEMMA_LOG_PROB}
            }
        };
        for (size_t i = 0; i < expectedArcs.size(); ++i) {
            UNIT_ASSERT_EQUAL(graph.GetArcsOnVertex(i), expectedArcs[i]);
        }
    }
}
