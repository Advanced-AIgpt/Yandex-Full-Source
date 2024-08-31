#pragma once

#include "utils.h"

#include <util/generic/algorithm.h>
#include <util/system/yassert.h>

#include <cstddef>
#include <utility>

namespace NBASS {
namespace NSmallGeo {

template <typename TTerm>
struct TTermFreq final {
    TTermFreq(const TTerm& term, double freq)
        : Term(term)
        , Freq(freq) {
    }

    bool operator<(const TTermFreq& rhs) const {
        if (Term != rhs.Term)
            return Term < rhs.Term;
        return Freq < rhs.Freq;
    }

    bool operator==(const TTermFreq& rhs) const {
        return Term == rhs.Term && Freq == rhs.Freq;
    }

    TTerm Term;
    double Freq;
};

template <typename TTerm, typename TFn>
static void ForEachCommon(const TVector<TTermFreq<TTerm>>& lhs, const TVector<TTermFreq<TTerm>>& rhs, TFn&& fn) {
    Y_ASSERT(IsSorted(lhs.begin(), lhs.end()));
    Y_ASSERT(IsSorted(rhs.begin(), rhs.end()));

    size_t i = 0;
    size_t j = 0;
    while (i < lhs.size() && j < rhs.size()) {
        if (lhs[i].Term < rhs[j].Term) {
            ++i;
        } else if (lhs[i].Term > rhs[j].Term) {
            ++j;
        } else {
            Y_ASSERT(lhs[i].Term == rhs[j].Term);
            fn(lhs[i], rhs[j]);
            ++i;
            ++j;
        }
    }
}

template <typename TTerm>
struct TTermFreqs {
    TTermFreqs() = default;

    template <typename TTerms>
    TTermFreqs(TTerms&& terms, double norm)
        : Terms(std::forward<TTerms>(terms))
        , Norm(norm) {
    }

    bool operator<(const TTermFreqs& rhs) const {
        if (Terms != rhs.Terms)
            return Terms < rhs.Terms;
        return Norm < rhs.Norm;
    }

    bool operator==(const TTermFreqs& rhs) const {
        return Terms == rhs.Terms && Norm == rhs.Norm;
    }

    TVector<TTermFreq<TTerm>> Terms;
    double Norm = 0;
};

} // namespace NSmallGeo
} // namespace NBASS
