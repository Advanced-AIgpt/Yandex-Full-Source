#include "catboost_item_selector.h"

#include <algorithm>

#include <kernel/lemmer/core/language.h>

#include <library/cpp/langmask/langmask.h>
#include <library/cpp/string_utils/levenshtein_diff/levenshtein_diff.h>

#include <contrib/libs/re2/re2/re2.h>

#include <util/charset/utf8.h>
#include <util/charset/wide.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/map.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/generic/ymath.h>
#include <util/string/builder.h>

namespace NAlice {
namespace NItemSelector {

namespace {

constexpr float MISSING_FEATURE_VALUE = -1;

using TBagOfWords = THashMap<TString, float>;
using TTagToBagOfWords = THashMap<TString, TBagOfWords>;

float GetWordToBagOfWordsEditQuasiDistance(const TString& word, const TBagOfWords& bow) {
    TMultiMap<int, float> distances;
    distances.insert({GetNumberOfUTF8Chars(word), 1});
    for (const auto& [wordTo, probability] : bow) {
        distances.insert({NLevenshtein::Distance(UTF8ToWide(word), UTF8ToWide(wordTo)), probability});
    }
    float score = 0;
    float accumulatedProbability = 1;
    for (const auto& [distance, probability] : distances) {
        score += probability * accumulatedProbability * distance;
        accumulatedProbability *= (1 - probability);
    }
    return score;
}

float GetProbabilisticEditQuasiDistance(const TBagOfWords& from, const TBagOfWords& to) {
    float score = 0;
    for (const auto& [word, probability] : from) {
        score += probability * GetWordToBagOfWordsEditQuasiDistance(word, to);
    }
    return score;
}

template <class TSomeMap>
THashSet<typename TSomeMap::key_type> GetMapKeysUnion(const TSomeMap& first, const TSomeMap& second) {
    THashSet<typename TSomeMap::key_type> keys;
    for (const auto& [key, value] : first) {
        keys.emplace(key);
    }
    for (const auto& [key, value] : second) {
        keys.emplace(key);
    }
    return keys;
}

float GetJaccardScore(const TBagOfWords& first, const TBagOfWords& second) {
    float firstScore = 0;
    float secondScore = 0;
    float intersectionScore = 0;
    for (const auto& key : GetMapKeysUnion(first, second)) {
        const float firstValue = first.Value(key, 0);
        const float secondValue = second.Value(key, 0);
        firstScore += firstValue;
        secondScore += secondValue;
        intersectionScore += firstValue * secondValue;
    }
    return intersectionScore / (firstScore + secondScore - intersectionScore);
}

THashMap<TString, float> ComputeSimilaritiesByTags(const TTagToBagOfWords& first,
                                                   const TTagToBagOfWords& second,
                                                   const TString& prefix) {
    static const TBagOfWords emptyBow;
    THashMap<TString, float> similarities;
    for (const auto& key : GetMapKeysUnion(first, second)) {
        const TBagOfWords firstValue = first.Value(key, emptyBow);
        const TBagOfWords secondValue = second.Value(key, emptyBow);
        similarities[prefix + "_jaccard_" + key] = GetJaccardScore(firstValue, secondValue);
        similarities[prefix + "_edit12_" + key] = GetProbabilisticEditQuasiDistance(firstValue, secondValue);
        similarities[prefix + "_edit21_" + key] = GetProbabilisticEditQuasiDistance(secondValue, firstValue);
    }
    return similarities;
}

// TODO(volobuev): maybe should use all lemmatizer results, should try to accept translit in lemmas
TString Lemmatize(const TString& word) {
    const TUtf16String wideWord = UTF8ToWide(word);
    TWLemmaArray lemmas;
    NLemmer::AnalyzeWord(wideWord.data(), wideWord.size(), lemmas, NLanguageMasks::BasicLanguages());
    return WideToUTF8(lemmas.front().GetText());
}

TString RemoveBioPrefix(const TString& tag) {
    return tag.StartsWith("B-") || tag.StartsWith("I-") ? tag.substr(2) : tag;
}

TTagToBagOfWords GetTagToBagOfWords(const TTaggerResult& taggerResult,
                                    const bool lemmatize,
                                    const bool removeBioPrefix) {
    TTagToBagOfWords tagsBow;
    for (const auto& alternative : taggerResult) {
        for (const auto& token : alternative.Tokens) {
            const TString tag = removeBioPrefix ? RemoveBioPrefix(token.Tag) : token.Tag;
            const TString text = lemmatize ? Lemmatize(token.Text) : token.Text;
            tagsBow[tag][text] += alternative.Probability;
        }
    }
    return tagsBow;
}

size_t CountNonRussianWords(const TTaggingAlternative& taggingAlternative) {
    size_t count = 0;
    static const RE2 pattern("[а-я]+|[0-9]+");
    for (const auto& token : taggingAlternative.Tokens) {
        count += !RE2::FullMatch(token.Text, pattern);
    }
    return count;
}

TFeatureMap ComputeBagOfWordsFeatureMap(const TCatboostSelectionRequest& request,
                                        const TCatboostSelectionItem& item,
                                        const bool removeBioPrefix) {
    TFeatureMap featureMap;

    const TTagToBagOfWords utteranceBagOfWords = GetTagToBagOfWords(request.TaggerResult,
                                                                    /* lemmatize = */ false,
                                                                    removeBioPrefix);
    Y_ENSURE(!item.TaggerResults.empty());
    const TTagToBagOfWords itemBagOfWords = GetTagToBagOfWords(item.TaggerResults[0],
                                                               /* lemmatize = */ false,
                                                               removeBioPrefix);
    TFeatureMap noLemmaSimilarities = ComputeSimilaritiesByTags(utteranceBagOfWords, itemBagOfWords,
                                                                /* prefix = */ "no_lemma");
    featureMap.insert(noLemmaSimilarities.begin(), noLemmaSimilarities.end());

    const TTagToBagOfWords utteranceLemmaBagOfWords = GetTagToBagOfWords(request.TaggerResult,
                                                                         /* lemmatize = */ true,
                                                                         removeBioPrefix);
    const TTagToBagOfWords itemLemmaBagOfWords = GetTagToBagOfWords(item.TaggerResults[0],
                                                                    /* lemmatize = */ true,
                                                                    removeBioPrefix);
    TFeatureMap lemmaSimilarities = ComputeSimilaritiesByTags(utteranceLemmaBagOfWords, itemLemmaBagOfWords,
                                                              /* prefix = */ "lemma");
    featureMap.insert(lemmaSimilarities.begin(), lemmaSimilarities.end());


    if (item.PositionTaggerResult) {
        const TTagToBagOfWords positionBagOfWords = GetTagToBagOfWords(item.PositionTaggerResult.GetRef(),
                                                                       /* lemmatize = */ true,
                                                                       removeBioPrefix);
        TFeatureMap positionSimilarities = ComputeSimilaritiesByTags(utteranceLemmaBagOfWords, positionBagOfWords,
                                                                     /* prefix = */ "position");
        featureMap.insert(positionSimilarities.begin(), positionSimilarities.end());
    }
    return featureMap;
}

const TTaggingAlternative& GetTopTagging(const TTaggerResult& tagging) {
    Y_ENSURE(!tagging.empty());
    return tagging[0];
}

TFeatureMap GetItemFeatures(const TCatboostSelectionItem& item) {
    if (const TVideoItemMeta* videoItemMeta = std::get_if<TVideoItemMeta>(&item.Meta); videoItemMeta) {
        const TFeatureMap features {
            {"duration", videoItemMeta->Duration},
            {"episode", videoItemMeta->Episode},
            {"episodes_count", videoItemMeta->EpisodesCount},
            {"min_age", videoItemMeta->MinAge},
            {"rating", videoItemMeta->Rating},
            {"release_year", videoItemMeta->ReleaseYear},
            {"season", videoItemMeta->Season},
            {"seasons_count", videoItemMeta->SeasonsCount},
            {"view_count", videoItemMeta->ViewCount},
            {"position", videoItemMeta->Position}
        };
        TFeatureMap nonzeroFeatures;
        for (const auto& [key, value] : features) {
            if (!FuzzyEquals(value + 1, 1)) {
                nonzeroFeatures[key] = value;
            }
        }
        return nonzeroFeatures;
    }
    return {};
}

}; // anonymous namespace

TFeatureMap ComputeFeatureMap(const TCatboostSelectionRequest& request,
                              const TCatboostSelectionItem& item,
                              const bool removeBioPrefix) {
    Y_ENSURE(!item.Synonyms.empty());
    Y_ENSURE(!item.TaggerResults.empty());
    Y_ENSURE(item.Synonyms.size() == item.TaggerResults.size(),
             "item has " << item.Synonyms.size() << "synonyms but " << item.TaggerResults.size() << "taggings");

    TFeatureMap featureMap = ComputeBagOfWordsFeatureMap(request, item, removeBioPrefix);
    const TFeatureMap itemFeatures = GetItemFeatures(item);
    featureMap.insert(itemFeatures.begin(), itemFeatures.end());
    {
        const TTaggingAlternative& topTagging = GetTopTagging(request.TaggerResult);

        featureMap["utterance_length"] = topTagging.Tokens.size();
        featureMap["utterance_non_russian_words_count"] = CountNonRussianWords(topTagging);
        featureMap["utterance_tagging_top_probability"] = topTagging.Probability;
    }
    {
        const TTaggingAlternative& topTagging = GetTopTagging(item.TaggerResults[0]);

        featureMap["item_length"] = topTagging.Tokens.size();
        featureMap["item_non_russian_words_count"] = CountNonRussianWords(topTagging);
        featureMap["item_tagging_top_probability"] = topTagging.Probability;
    }
    if (item.PositionTaggerResult) {
        const TTaggingAlternative& topTagging = GetTopTagging(item.PositionTaggerResult.GetRef());

        featureMap["position_tagging_top_probability"] = topTagging.Probability;
    }
    return featureMap;
}

TVector<float> ComputeFeatures(const TCatboostSelectionRequest& request,
                               const TCatboostSelectionItem& item,
                               const TCatboostItemSelectorSpec spec) {
    const TFeatureMap featureMap = ComputeFeatureMap(request, item, spec.RemoveBioPrefix);
    TVector<float> features(spec.FeatureVectorSize);
    for (const auto& [key, value] : spec.FeatureSpec) {
        Y_ENSURE(0 <= value && value < spec.FeatureVectorSize,
                "feature index " << value << " is out of range [0; " << spec.FeatureVectorSize << ")");
        features[value] = featureMap.Value(key, MISSING_FEATURE_VALUE);
    }
    return features;
}

TVector<TSelectionResult> TCatboostItemSelector::Select(const TCatboostSelectionRequest& request,
                                                        const TVector<TCatboostSelectionItem>& items) const {
    TVector<TVector<float>> features;
    for (const auto& item : items) {
        features.push_back(ComputeFeatures(request, item, Spec));
    }
    TVector<double> relevancies(items.size());
    CatboostModel.CalcFlat(features, relevancies);
    TVector<TSelectionResult> result;

    const size_t maxRelevanceIndex = std::max_element(relevancies.begin(), relevancies.end()) - relevancies.begin();

    for (size_t i = 0; i < relevancies.size(); ++i) {
        const double probability = Sigmoid(relevancies[i]);
        result.push_back({probability, i == maxRelevanceIndex && probability > Spec.SelectionThreshold});
    }
    return result;
}

TVector<TSelectionResult> TEasyCatboostItemSelector::Select(const TSelectionRequest& request,
                                                            const TVector<TSelectionItem>& items) const {
    TCatboostSelectionRequest catboostRequest(request, EasyTagger.Tag(request.Phrase.Text));
    TVector<TCatboostSelectionItem> catboostItems;
    catboostItems.reserve(items.size());
    for (auto& item : items) {
        TVector<TTaggerResult> taggerResults;
        taggerResults.reserve(item.Synonyms.size());
        for (auto& synonym : item.Synonyms) {
            taggerResults.emplace_back(EasyTagger.Tag(synonym.Text, synonym.Language));
        }
        if (const TVideoItemMeta* videoItemMeta = std::get_if<TVideoItemMeta>(&item.Meta); videoItemMeta) {
            catboostItems.emplace_back(item, taggerResults, EasyTagger.Tag(GetPositionalText(videoItemMeta->Position)));
        } else {
            catboostItems.emplace_back(item, taggerResults, Nothing());
        }
    }
    return CatboostItemSelector.Select(catboostRequest, catboostItems);
}

} // namespace NItemSelector
} // namespace NAlice
