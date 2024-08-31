#pragma once

#include <alice/nlu/granet/lib/sample/entity.h>

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice {

inline const TString DSSM_EMBEDDING_NAME = "dssm";
inline const TString BEGGINS_EMBEDDING_NAME = "beggins";
inline const TString ZELIBOBA_EMBEDDING_NAME = "zeliboba";

struct TMixedBinaryClassifierInput {
    TString Text;

    // Embedding name to vector
    THashMap<TString, TVector<float>> SentenceEmbeddings;

    // Feature name to value
    THashMap<TString, float> SentenceFeatures;
};

THashMap<TString, float> CalculateSentenceFeatures(TStringBuf text,
    const TVector<NGranet::TEntity>& entities);

} // namespace NAlice
