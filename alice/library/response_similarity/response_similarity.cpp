#include "response_similarity.h"

#include <alice/library/parsed_user_phrase/utils.h>
#include <alice/nlu/libs/request_normalizer/request_normalizer.h>

#include <util/generic/algorithm.h>

using namespace NParsedUserPhrase;

namespace NAlice::NResponseSimilarity {

namespace {

void InitStatistics(const float initValue, TSimilarity::TSimilarityStatistics* statistics) {
    statistics->SetMax(initValue);
    statistics->SetMean(initValue);
    statistics->SetMin(initValue);
};

void UpdateStatistic(const TSimilarity::TSimilarityStatistics& statistic, TSimilarity::TSimilarityStatistics& accumulator) {
    accumulator.SetMax(Max(accumulator.GetMax(), statistic.GetMax()));
    accumulator.SetMean(accumulator.GetMean() + statistic.GetMean());
    accumulator.SetMin(Min(accumulator.GetMin(), statistic.GetMin()));
}

} // namespace

TSimilarity CalculateNormalizedResponseItemSimilarity(
        const TParsedSequence& query,
        const TStringBuf normalizedResponse)
{
    TSimilarity result;
    TParsedSequence document(normalizedResponse);

    InitStatistics(Max(query.Match(document), 0.f), result.MutableQueryInResponse());
    InitStatistics(Max(document.Match(query), 0.f), result.MutableResponseInQuery());

    size_t prefixLength = query.Size();
    InitStatistics(Max(document.MatchPrefix(query, prefixLength), 0.f), result.MutablePrefix());
    InitStatistics(Max(document.MatchPrefix(query, 2 * prefixLength), 0.f), result.MutableDoublePrefix());

    return result;
}

TSimilarity CalculateResponseItemSimilarity(
        const TParsedSequence& query,
        const TStringBuf response,
        const ELanguage lang)
{
    const TString normalizedResponse = NNlu::TRequestNormalizer::Normalize(lang, response);
    return CalculateNormalizedResponseItemSimilarity(query, normalizedResponse);
}

TSimilarity CalculateResponseItemSimilarity(
        const TStringBuf searchText,
        const TStringBuf response,
        const ELanguage lang)
{
    return CalculateResponseItemSimilarity(TParsedSequence(searchText), response, lang);
}

TSimilarity AggregateSimilarity(const TVector<TSimilarity>& similarities) {
    TSimilarity result;
    for (const auto& item : similarities) {
        UpdateStatistic(item.GetQueryInResponse(), *result.MutableQueryInResponse());
        UpdateStatistic(item.GetResponseInQuery(), *result.MutableResponseInQuery());
        UpdateStatistic(item.GetPrefix(), *result.MutablePrefix());
        UpdateStatistic(item.GetDoublePrefix(), *result.MutableDoublePrefix());
    }
    if (!similarities.empty()) {
        result.MutableQueryInResponse()->SetMean(result.MutableQueryInResponse()->GetMean() / similarities.size());
        result.MutableResponseInQuery()->SetMean(result.MutableResponseInQuery()->GetMean() / similarities.size());
        result.MutablePrefix()->SetMean(result.MutablePrefix()->GetMean() / similarities.size());
        result.MutableDoublePrefix()->SetMean(result.MutableDoublePrefix()->GetMean() / similarities.size());
    }
    return result;
}

} // namespace NAlice::NResponseSimilarity
