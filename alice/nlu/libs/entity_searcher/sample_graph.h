#pragma once

#include "entity_searcher_types.h"

#include <alice/nlu/granet/lib/sample/sample.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice::NNlu {

class TSampleGraph {
public:
    struct TArc {
        size_t To;
        TString Token;
        double LogProbability;
        DECLARE_TUPLE_LIKE_TYPE(TArc, To, Token, LogProbability);
    };

    struct TVertex {
        TVector<TArc> Arcs;
    };

    TSampleGraph(const TVector<TString>& tokens, const TVector<TString>& lemmas, const TVector<TSynonym>& synonyms);

    const TVector<TSampleGraph::TArc>& GetArcsOnVertex(size_t vertexId) const;

    size_t Size() const;

    static TSampleGraph FromGranetSample(const NGranet::TSample::TConstRef& sample, bool addSynonyms, bool addNominativesAsLemma = false);

private:
    void AddTokenArcs(const TVector<TString>& tokens);
    void AddLemmaArcs(const TVector<TString>& lemmas);
    void AddSynonymArcs(const TVector<TSynonym>& synonyms);
    void AddSynonymArc(const TSynonym& synonym);
    void AddTokenArc(const ::NNlu::TInterval& interval, TStringBuf token, double logProb);
    void RemoveDuplicates();
    TVector<TVertex> Vertices;
};

} // namespace NAlice::NNlu
