#include "engine.h"

#include "math.h"

#include <library/cpp/geolocation/calcer.h>

#include <util/datetime/cputimer.h>
#include <util/generic/algorithm.h>
#include <util/generic/utility.h>
#include <util/stream/input.h>
#include <util/string/join.h>
#include <util/system/yassert.h>
#include <util/ysaveload.h>

#include <cmath>
#include <utility>

using namespace std;

namespace NAlice::NSmallGeo {
namespace {
template <typename T, typename TFn>
TMaybe<invoke_result_t<TFn, T>> LiftM(const TMaybe<T>& maybe, TFn&& fn) {
    if (maybe)
        return fn(*maybe);
    return Nothing();
}

double DistanceOnEarthMeters(const TLatLon& u, const TLatLon& v) {
    return NGeolocationFeatures::CalcDistance(u.Lat, u.Lon, v.Lat, v.Lon);
}

} // namespace

// static
const TEngine::TTermId TEngine::INVALID_TERM_ID;

TEngine::TEngine(const TVector<TRegion>& regions)
    : Regions(regions)
{
    for (size_t i = 0; i < Regions.size(); ++i) {
        IdToIndex[Regions[i].Id] = i;
    }

    for (size_t regionIndex = 0; regionIndex < Regions.size(); ++regionIndex) {
        const auto& region = Regions[regionIndex];

        region.ForEachToken([this, regionIndex](TStringBuf token) {
            const TTermId termId = Vocabulary.AddGetId(token);
            Index[termId].push_back(regionIndex);
        });
    }

    for (auto& kv : Index) {
        SortUnique(kv.second);
    }

    Vectors.resize(Regions.size());
    for (size_t regionIndex = 0; regionIndex < Regions.size(); ++regionIndex) {
        const auto& region = Regions[regionIndex];
        region.ForEachName(
            [this, regionIndex](TStringBuf name) { Vectors[regionIndex].push_back(BuildTermFreqs(name)); });
        SortUnique(Vectors[regionIndex]);
    }
}

TVector<TResult> TEngine::FindTopRegions(TStringBuf query, const TMaybe<TLatLon>& position, size_t top) const {
    const auto q = BuildTermFreqs(query);

    TVector<size_t> all;
    for (const auto& term : q.Terms) {
        const auto it = Index.find(term.Term);
        if (it != Index.end())
            all.insert(all.end(), it->second.begin(), it->second.end());
    }

    SortUnique(all);

    TVector<TResult> results;
    for (const auto& regionIndex : all) {
        const auto& region = Regions[regionIndex];

        const auto distanceM =
            LiftM(position, [&](const TLatLon& p) { return DistanceOnEarthMeters(p, region.Center); });

        const auto sim = static_cast<int>(TResult::MaxSimilarity * Similarity(q, regionIndex));
        results.emplace_back(regionIndex, ClampVal(sim, 0, TResult::MaxSimilarity), region.Type, distanceM);
    }

    Sort(results, [&](const TResult& lhs, const TResult& rhs) {
        if (lhs.Similarity != rhs.Similarity)
            return lhs.Similarity > rhs.Similarity;

        if (lhs.DistanceM && !rhs.DistanceM)
            return true;
        if (!lhs.DistanceM && rhs.DistanceM)
            return false;
        if (lhs.DistanceM && rhs.DistanceM)
            return *lhs.DistanceM < *rhs.DistanceM;

        return lhs.Index < rhs.Index;
    });

    if (results.size() > top)
        results.erase(results.begin() + top, results.end());

    return results;
}

TString TEngine::GetExtendedName(size_t regionIndex) const {
    TVector<TStringBuf> names;
    GetPathToRoot(regionIndex, [&](const TRegion& region) {
        if (region.Type >= 3 && !region.Name.empty())
            names.push_back(region.Name);
    });
    return JoinSeq(", ", names);
}

TEngine::TTermFreqs TEngine::BuildTermFreqs(TStringBuf name) const {
    TVector<TString> tokens;
    ForEachToken(name, [&tokens](const TString& token) { tokens.push_back(token); });
    Sort(tokens);

    TVector<TTermFreq> terms;

    size_t i = 0;
    while (i < tokens.size()) {
        size_t j = i + 1;
        while (j < tokens.size() && tokens[i] == tokens[j])
            ++j;

        Y_ASSERT(j != i);
        Y_ASSERT(tokens.size() != 0);

        const double tf = static_cast<double>(j - i);

        if (Vocabulary.Has(tokens[i]))
            terms.emplace_back(Vocabulary.GetId(tokens[i]), tf);
        else
            terms.emplace_back(INVALID_TERM_ID, tf);

        i = j;
    }

    Sort(terms);

    const double norm = GetNorm(terms);

    return {move(terms), norm};
}

double TEngine::GetWeight(const TTermFreq& term) const {
    const double tf = term.Freq;

    double df = 0.5;
    const auto it = Index.find(term.Term);
    if (it != Index.end())
        df = static_cast<double>(it->second.size()) / Regions.size();

    return (1 + log2(tf)) * log2(1.0 / df);
}

double TEngine::GetNorm(const TVector<TTermFreq>& terms) const {
    double sum = 0;

    for (const auto& term : terms) {
        const double w = GetWeight(term);
        sum += w * w;
    }

    return sqrt(sum);
}

double TEngine::Similarity(const TTermFreqs& query, const TTermFreqs& doc) const {
    double nom = 0;

    ForEachCommon(query.Terms, doc.Terms, [this, &nom](const TTermFreq& lhs, const TTermFreq& rhs) {
        nom += GetWeight(lhs) * GetWeight(rhs);
    });

    const double denom = query.Norm * doc.Norm;

    return denom < 1e-9 ? 0 : nom / denom;
}

double TEngine::Similarity(const TTermFreqs& query, size_t regionIndex) const {
    double similarity = 0;

    Y_ASSERT(regionIndex < Regions.size());
    for (const auto& doc : Vectors[regionIndex])
        similarity = max(similarity, Similarity(query, doc));
    return similarity;
}

template <typename TFn>
void TEngine::GetPathToRoot(size_t regionIndex, const TFn& fn) const {
    Y_ASSERT(regionIndex < Regions.size());
    const auto region = Regions[regionIndex];
    if (region.Id == 0)
        return;

    fn(region);

    const auto it = IdToIndex.find(region.ParentId);
    Y_ASSERT(it != IdToIndex.end());
    GetPathToRoot(it->second, fn);
}

} // namespace NAlice::NSmallGeo
