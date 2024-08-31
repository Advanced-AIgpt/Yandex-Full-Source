#include "direct.h"

#include <library/cpp/langs/langs.h>
#include <alice/library/parsed_user_phrase/parsed_sequence.h>
#include <alice/library/response_similarity/response_similarity.h>

#include <alice/megamind/protos/scenarios/features/search.pb.h>

#include <util/generic/strbuf.h>
#include <util/generic/vector.h>

namespace NAlice::NHollywood::NSearch {

namespace {

void GetDirectTexts(const TSearchResult& search, TVector<TStringBuf>& offerTexts,
                    TVector<TStringBuf>& titleTexts, TVector<TStringBuf>& infoTexts)
{
    // For example renderrer response JSON look here:
    // https://paste.yandex-team.ru/2025329

    for (const auto& doc : search.RenderrerResponse["docs"].GetArray()) {
        for (const auto& card : doc["scenario_response"]["layout"]["cards"].GetArray()) {
            if (!card.Has("div2_card_extended")) {
                continue;
            }
            for (const auto& state : card["div2_card_extended"]["body"]["states"].GetArray()) {
                // Now iterating different ads
                for (const auto& item : state["div"]["items"].GetArray()) {
                    const auto& dataBlocks = item["items"].GetArray();
                    if (dataBlocks.size() > 0) {
                        if (const auto& offer = dataBlocks[0]["text"].GetString()) {
                            offerTexts.push_back(offer);
                        }
                    }
                    if (dataBlocks.size() > 2) {
                        const auto& dataSubblocks = dataBlocks[2]["items"].GetArray();
                        if (dataSubblocks.size() > 0) {
                            if (const auto& title = dataSubblocks[0]["text"].GetString()) {
                                titleTexts.push_back(title);
                            }
                        }
                        if (dataSubblocks.size() > 1) {
                            if (const auto& info = dataSubblocks[1]["text"].GetString()) {
                                infoTexts.push_back(info);
                            }
                        }
                    }
                }
            }
        }
    }
}

NResponseSimilarity::TSimilarity GetAggregatedQueryAndTextsSimilarity(const NParsedUserPhrase::TParsedSequence& query, const TVector<TStringBuf>& texts, const ELanguage lang) {
    TVector<NResponseSimilarity::TSimilarity> similarities;
    similarities.reserve(texts.size());
    for (const TStringBuf text : texts) {
        similarities.emplace_back(NResponseSimilarity::CalculateResponseItemSimilarity(query, text, lang));
    }
    return AggregateSimilarity(similarities);
}

} // namespace

void CalculateDirectFactors(NSearch::TSearchContext& context, const TSearchResult& search) {
    TVector<TStringBuf> offerTexts;
    TVector<TStringBuf> titleTexts;
    TVector<TStringBuf> infoTexts;
    GetDirectTexts(search, offerTexts, titleTexts, infoTexts);

    const ELanguage lang = LanguageByName(context.GetLangName());
    const auto query = NParsedUserPhrase::TParsedSequence(context.GetQuery());

    *context.GetFeatures().MutableDirectOfferSimilarity() = GetAggregatedQueryAndTextsSimilarity(query, offerTexts, lang);
    *context.GetFeatures().MutableDirectTitleSimilarity() = GetAggregatedQueryAndTextsSimilarity(query, titleTexts, lang);
    *context.GetFeatures().MutableDirectInfoSimilarity() = GetAggregatedQueryAndTextsSimilarity(query, infoTexts, lang);
}

} // namespace NAlice::NHollywood::NSearch
