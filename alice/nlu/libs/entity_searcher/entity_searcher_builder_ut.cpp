#include "entity_searcher_builder.h"
#include "entity_searcher_utils.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/vector.h>
#include <util/string/split.h>

using namespace NAlice::NNlu;

Y_UNIT_TEST_SUITE(EntitySearcherDataBuilder) {
    Y_UNIT_TEST(TestEntitySearcherDataBuilder) {
        const TVector<TEntityString> entities = {
            {
                .Sample = "здравствуйте",
                .Type = "type1",
                .Value = "value1",
                .LogProbability = -2.
            },
            {
                .Sample = "до свидания",
                .Type = "type1",
                .Value = "value2",
                .LogProbability = -1.
            },
            {
                .Sample = "как дела",
                .Type = "type2",
                .Value = "value1",
                .LogProbability = -5.
            },
            {
                .Sample = "здравствуйте еще раз",
                .Type = "type1",
                .Value = "value1"
            },
            {
                .Sample = "еще раз здравствуйте",
                .Type = "type1",
                .Value = "value1"
            }
        };
        const TEntitySearcherData data = TEntitySearcherDataBuilder().Build(entities);
        THashMap<TString, TTokenId> stringToId;
        const TVector<TString>& idToString = data.IdToString;
        for (TTokenId id = 0; id < idToString.size(); ++id) {
            stringToId[idToString[id]] = id;
        }
        const TTrie trie = TTrie(data.TrieData);
        for (const TEntityString& entity : entities) {
            TVector<TTokenId> indexes = GetTokenIds(StringSplitter(entity.Sample).Split(' '), &stringToId);
            TVector<size_t> entityStringIndexes;
            UNIT_ASSERT(trie.Find(indexes, &entityStringIndexes));
            bool found = false;
            for (size_t id : entityStringIndexes) {
                if (entities[id] == entity) {
                    found = true;
                }
            }
            UNIT_ASSERT(found);
        }
    }
}
