#include "factors.h"

#include <alice/library/parsed_user_phrase/parsed_sequence.h>
#include <alice/library/parsed_user_phrase/utils.h>

#include <util/string/builder.h>

namespace NVideoCommon {
namespace NHasGoodResult {
namespace {
void AppendNameRange(TStringBuf prefix, size_t from, size_t to, TVector<TString>& names) {
    for (size_t i = from; i < to; ++i)
        names.push_back(TStringBuilder() << prefix << i);
}

void AppendNameRange(TStringBuf prefix, size_t n, TVector<TString>& names) {
    AppendNameRange(prefix, 0, n, names);
}

template <size_t N>
void AppendValues(const float (&values)[N], TVector<float>& result) {
    for (const auto& value : values)
        result.push_back(value);
}

template <size_t N>
void AppendDivValues(const float (&values)[N], TVector<float>& result) {
    constexpr float EPS = 1e-6;

    for (size_t i = 1; i < N; ++i) {
        if (values[0] < EPS)
            result.emplace_back(1);
        else
            result.emplace_back(values[i] / values[0]);
    }
}
} // namespace

// static
float TFactors::CalcSimilarity(TStringBuf query, TStringBuf name) {
    const NParsedUserPhrase::TParsedSequence q(query);
    const NParsedUserPhrase::TParsedSequence n(name);
    return q.Match(n);
}

// static
void TFactors::GetNames(TVector<TString>& names) {
    for (const auto& prefix : {"Rel@", "RelPred@", "Sim@"})
        AppendNameRange(prefix, TOP_RESULTS, names);

    AppendNameRange("DivRel@0", 1, TOP_RESULTS, names);
    AppendNameRange("DivRelPred@0", 1, TOP_RESULTS, names);
}

void TFactors::GetValues(TVector<float>& values) const {
    AppendValues(Relevances, values);
    AppendValues(RelevancePredictions, values);
    AppendValues(Similarities, values);

    AppendDivValues(Relevances, values);
    AppendDivValues(RelevancePredictions, values);
}

void TFactors::Clear() {
    for (size_t i = 0; i < TOP_RESULTS; ++i) {
        Relevances[i] = 0;
        RelevancePredictions[i] = 0;
        Similarities[i] = 0;
    }
}
} // namespace NHasGoodResult
} // namespace NVideoCommon
