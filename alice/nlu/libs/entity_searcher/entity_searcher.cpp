#include "entity_searcher.h"

#include "entity_searcher_utils.h"

#include <library/cpp/containers/comptrie/search_iterator.h>

#include <util/digest/multi.h>
#include <util/string/split.h>

namespace NAlice::NNlu {

TEntitySearcher::TEntitySearcher(const TEntitySearcherData& data)
    : Trie(data.TrieData), EntityStrings(data.EntityStrings)
{
    for (TTokenId i = 0; i < data.IdToString.size(); ++i) {
        StringToId[data.IdToString[i]] = i;
    }
}

TMaybe<TTrieState> TEntitySearcher::GoDownTheArc(const TSampleGraph::TArc& arc, const TTrieState& state) const {
    TTrieIterator next = state.PositionInTrie;
    TVector<TTokenId> arcIds = GetTokenIds(StringSplitter(arc.Token).Split(' '), &StringToId);
    if (next.Advance(arcIds)) {
        return TTrieState{
            .PositionInTrie = next,
            .LogProbability = state.LogProbability + arc.LogProbability
        };
    }
    return TMaybe<TTrieState>();
}

void TEntitySearcher::AddEntity(size_t start, size_t end, const TString& type, const TString& value,
                                double logProb, double quality, TVector<NGranet::TEntity>* entities) const {
    entities->push_back(
        NGranet::TEntity{
            .Interval = {start, end},
            .Type = type,
            .Value = value,
            .LogProbability = logProb,
            .Quality = quality
        }
    );
}

void TEntitySearcher::AddEntities(size_t start, size_t end, const TTrieIterator& pos,
                                  double logProb, TVector<NGranet::TEntity>* entities) const {
    TVector<size_t> stringIndexes;
    pos.GetValue(&stringIndexes);
    for (size_t stringIndex : stringIndexes) {
        const TEntityString& entityString = EntityStrings[stringIndex];
        AddEntity(start, end, entityString.Type, entityString.Value, logProb + entityString.LogProbability,
                  entityString.Quality, entities);
    }
}

void TEntitySearcher::SearchFromPosition(size_t start, const TSampleGraph& request,
                                         TVector<NGranet::TEntity>* entities) const {
    TVector<THashSet<TTrieState>> states(request.Size());
    states[start].insert(TTrieState{
        .PositionInTrie = MakeSearchIterator(Trie), 
        .LogProbability = 0.
    });
    for (size_t vertex = start; vertex < request.Size(); ++vertex) {
        for (const auto& state : states[vertex]) {
            AddEntities(start, vertex, state.PositionInTrie, state.LogProbability, entities);
            for (const auto& arc : request.GetArcsOnVertex(vertex)) {
                Y_ASSERT(vertex < arc.To && arc.To < request.Size());
                if (auto next = GoDownTheArc(arc, state); next.Defined()) {
                    states[arc.To].insert(next.GetRef());
                }
            }
        }
    }
}

TVector<NGranet::TEntity> TEntitySearcher::Search(const TSampleGraph& request) const {
    TVector<NGranet::TEntity> entities;
    for (size_t start = 0; start + 1 < request.Size(); ++start) {
        SearchFromPosition(start, request, &entities);
    }
    return entities;
}

} // namespace NAlice::NNlu
