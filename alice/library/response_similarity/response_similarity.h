#pragma once

#include <alice/library/parsed_user_phrase/parsed_sequence.h>
#include <alice/library/response_similarity/proto/similarity.pb.h>
#include <library/cpp/langs/langs.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice::NResponseSimilarity {

TSimilarity CalculateNormalizedResponseItemSimilarity(const NParsedUserPhrase::TParsedSequence& query, const TStringBuf normalizedResponse);
TSimilarity CalculateResponseItemSimilarity(const NParsedUserPhrase::TParsedSequence& query, const TStringBuf response, const ELanguage lang);
TSimilarity CalculateResponseItemSimilarity(const TStringBuf searchText, const TStringBuf response, const ELanguage lang);

TSimilarity AggregateSimilarity(const TVector<TSimilarity>& similarities);

} // namespace NAlice::NResponseSimilarity
