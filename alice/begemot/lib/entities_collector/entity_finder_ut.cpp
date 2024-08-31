#include "entity_finder.h"

#include <alice/nlu/granet/lib/sample/entity.h>
#include <alice/nlu/granet/lib/sample/entity_utils.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NBg::NAliceEntityCollector {

struct TTestSample {
    TVector<TString> Winners;
    TVector<TString> Matches;
    TVector<NGranet::TEntity> ExpectedEntities;
};

TVector<TTestSample> TEST_SAMPLES = {
    {
        {
            "убить билла\t1\t3\truw229250\t0.807\tfilm\tfb:award.award_nominated_work|fb:award.award_winning_work|fb:film.film\t3",
            "убить билла\t1\t3\truw229224\t0.699\tfilm\t\t3"
        },
        {
            "включи\t0\t1\tyam04171314\t0.116\tmusic\t",
            "убить билла\t1\t3\tyam014384\t0.016\tmusic\t",
            "включи\t0\t1\tyam339174305\t0.014\tmusic\t",
            "включи\t0\t1\tyam08695285\t0.000\tmusic\t",
            "включи\t0\t1\tyam08638694\t0.000\tmusic\t"
        },
        {
            {
                .Interval = {1, 3},
                .Type = "entity_search.film",
                .Value = "ruw229250",
                .LogProbability = NGranet::NEntityLogProbs::ENTITY_SEARCH,
                .Quality = 0.807,
            }
        }
    },
    {
        {
        },
        {
            "включи\t0\t1\tyam08695285\t0.000\tmusic\t",
            "включи\t0\t1\tyam339174305\t0.014\tmusic\t",
            "включи\t0\t1\tyam04171314\t0.116\tmusic\t",
            "убить билла\t1\t3\tyam014384\t0.016\tmusic\t",
            "включи\t0\t1\tyam08638694\t0.000\tmusic\t"
        },
        {
            {
                .Interval = {1, 3},
                .Type = "entity_search.music",
                .Value = "yam014384",
                .LogProbability = NGranet::NEntityLogProbs::ENTITY_SEARCH,
                .Quality = 0.016,
            },
            {
                .Interval = {0, 1},
                .Type = "entity_search.music",
                .Value = "yam04171314",
                .LogProbability = NGranet::NEntityLogProbs::ENTITY_SEARCH,
                .Quality = 0.116,
            }
        }
    },
    {
        {
        },
        {
        },
        {
        }
    }
};

Y_UNIT_TEST_SUITE(EntityFinder) {
    Y_UNIT_TEST(TestEntitiesCollection) {
        for (const TTestSample& testSample : TEST_SAMPLES) {
            NProto::TEntityFinderResult input;

            for (const TString& match : testSample.Winners) {
                input.AddWinner(match);
            }
            for (const TString& match : testSample.Matches) {
                input.AddMatch(match);
            }

            const auto collectedEntities = CollectEntityFinder(input);

            UNIT_ASSERT_VALUES_EQUAL(collectedEntities.size(), testSample.ExpectedEntities.size());
            for (size_t entityIndex = 0; entityIndex < collectedEntities.size(); ++entityIndex) {
                UNIT_ASSERT_VALUES_EQUAL(collectedEntities[entityIndex], testSample.ExpectedEntities[entityIndex]);
            }
        }
    }
}

} // namespace NBg::NAliceEntityCollector
