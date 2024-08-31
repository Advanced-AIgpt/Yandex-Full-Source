#pragma once

#include "latlon.h"
#include "region.h"
#include "result.h"
#include "term_freq.h"
#include "vocabulary.h"

#include <util/datetime/base.h>
#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/system/types.h>

#include <cstddef>
#include <limits>

class IInputStream;

namespace NAlice::NSmallGeo {

// For each region we put all tokens from region name, synonyms and cases
// to inverted index. When search is performed, corresponding lists of
// regions from inverted index are concatenated, and then sorted
// according to the simple TF-IDF vector model.
//
// Note that during ranking, each region is represented not as a single
// vector, but as a set of vectors corresponding to region names. So,
// similarity between a query vector and a region is the maximum of
// similarities between the query vector and each vector corresponding to
// the region.
//
// Also, when two regions have same tf-idf similarities to the query
// vector, the one that is closest to the user position is ranked
// higher. This is the poor-man version of the linear regression model,
// with two factors - tf-idf similarity and distance to the user, when
// the coefficient for tf-idf similarity is infinitely larger than the
// coefficient for distance.
class TEngine {
public:
    using TRegionIndex = size_t;

public:
    explicit TEngine(const TVector<TRegion>& regions);

    TVector<TResult> FindTopRegions(TStringBuf query, const TMaybe<TLatLon>& position, size_t top) const;

    TString GetExtendedName(TRegionIndex regionIndex) const;

private:
    using TTermId = ui64;

    using TTermFreq = TTermFreq<TTermId>;
    using TTermFreqs = TTermFreqs<TTermId>;

    static constexpr TTermId INVALID_TERM_ID = std::numeric_limits<TTermId>::max();

private:
    TTermFreqs BuildTermFreqs(TStringBuf name) const;

    double GetWeight(const TTermFreq& term) const;
    double GetNorm(const TVector<TTermFreq>& terms) const;

    double Similarity(const TTermFreqs& query, const TTermFreqs& doc) const;
    double Similarity(const TTermFreqs& query, size_t regionIndex) const;

    template <typename TFn>
    void GetPathToRoot(size_t regionIndex, const TFn& fn) const;

private:
    const TVector<TRegion>& Regions;

    THashMap<TRegion::TId, TRegionIndex> IdToIndex;

    TVocabulary Vocabulary;
    THashMap<TTermId, TVector<TRegionIndex>> Index;

    TVector<TVector<TTermFreqs>> Vectors;
};

} // namespace NAlice::NSmallGeo
