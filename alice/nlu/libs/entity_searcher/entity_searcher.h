#pragma once

#include "entity_searcher_types.h"
#include "sample_graph.h"

#include <alice/nlu/granet/lib/sample/entity.h>

#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice::NNlu {

struct TTrieState {
    TTrieIterator PositionInTrie;
    double LogProbability;
    
    bool operator==(const TTrieState& o) const {
        return std::tie(PositionInTrie, LogProbability) == std::tie(o.PositionInTrie, o.LogProbability);
    }

    size_t GetHash() const {
        return MultiHash(
            PositionInTrie,
            LogProbability
        );
    }
};

class TEntitySearcher {
public:
    explicit TEntitySearcher(const TEntitySearcherData& data);

    TVector<NGranet::TEntity> Search(const TSampleGraph& request) const;

private:
    void SearchFromPosition(size_t start, const TSampleGraph& request, TVector<NGranet::TEntity>* entities) const;
    TMaybe<TTrieState> GoDownTheArc(const TSampleGraph::TArc& arc, const TTrieState& state) const;
    void AddEntity(size_t start, size_t end, const TString& type, const TString& value, double logProb,
                   double quality, TVector<NGranet::TEntity>* entities) const;
    void AddEntities(size_t start, size_t end, const TTrieIterator& pos, double logProb,
                     TVector<NGranet::TEntity>* entities) const;

private:
    TTrie Trie;
    TVector<TEntityString> EntityStrings;
    THashMap<TString, TTokenId> StringToId;
};

} // namespace NAlice::NNlu

template<>
struct THash<NAlice::NNlu::TTrieState> {
    size_t operator()(const NAlice::NNlu::TTrieState& item) const {
        return item.GetHash();
    }
};
