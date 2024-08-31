#include "feature_extractor.h"

#include <alice/nlu/granet/lib/sample/entity_utils.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/split.h>
#include <algorithm>
#include <library/cpp/iterator/zip.h>

using namespace NBg;

namespace {
    struct TEntityFinderParsedResult {
        int Begin;
        int End;
        double Weight;
        TString OntoCategory;
        TString FreeBaseCategories;
    };

    TString JoinTokens(const NProto::TExternalMarkupProto& externalMarkup) {
        TStringBuilder text;
        for (const NProto::TExternalMarkupProto::TToken& token : externalMarkup.GetTokens()) {
            if (!text.Empty()) {
                text << ' ';
            }
            text << token.GetText();
        }
        return text;
    }
} // namespace

static const TString UNKNOWN_POS_TAG = "UNKN";

static const THashMap<TString, TString> POS_TAG_MAPPING = {
    {"A", "ADJF"},
    {"ADV", "ADVB"},
    {"ANUM", "NUMR"},
    {"ADVPRO", "PRED"},
    {"partcp", "PRTF"},
    {"ger", "GRND"},
    {"PR", "PREP"},
    {"SPRO", "NPRO"},
    {"INTJ", "INTJ"},
    {"PART", "PRCL"},
    {"S", "NOUN"},
    {"NUM", "NUMR"},
    {"V", "VERB"},
    {"inf", "INFN"},
    {"CONJ", "CONJ"},
    {UNKNOWN_POS_TAG, UNKNOWN_POS_TAG}
};

 NNlu::TInterval TFeatureExtractor::GetInterval(size_t begin, size_t end, const NNlu::TAlignment& alignment) {
    return alignment.GetMap1To2().ConvertInterval({begin, end});
}

NVins::TSampleFeatures TFeatureExtractor::ExtractWordEmbeddingsFeatures(
    const NProto::TAliceEmbeddingsResult& embeddingsProto
) const {
    NVins::TSampleFeatures sampleFeatures;

    TVector<TVector<float>>& embeddings = sampleFeatures.DenseSeq["alice_requests_emb"];
    embeddings.reserve(embeddingsProto.GetEmbedding().size());
    sampleFeatures.Sample.Tokens.reserve(embeddingsProto.GetEmbedding().size());

    for (const auto& embeddingProto : embeddingsProto.GetEmbedding()) {
        sampleFeatures.Sample.Tokens.push_back(embeddingProto.GetText());
        embeddings.emplace_back(embeddingProto.GetValue().begin(), embeddingProto.GetValue().end());
    }

    return sampleFeatures;
}

void TFeatureExtractor::AddFeatureSpan(
    const TString& featureName,
    size_t begin,
    size_t end,
    TVector<TVector<NVins::TSparseFeature>>* features
) const {
    if (begin < 0 || end > features->size()) {
        return;
    }

    TString bioPrefix = "B-";
    for (size_t tokenIndex = begin; tokenIndex < end; ++tokenIndex) {
        (*features)[tokenIndex].emplace_back(bioPrefix + featureName);
        bioPrefix = "I-";
    }
}

void TFeatureExtractor::ExtractCustomEntities(
    const NProto::TCustomEntitiesResult* customEntitiesResult,
    const NNlu::TAlignment& alignment,
    TVector<TVector<NVins::TSparseFeature>>* nerFeatures
) const {
    if (!customEntitiesResult) {
        return;
    }

    const auto& occurrenceRanges = customEntitiesResult->GetOccurrences().GetRanges();
    const auto& occurrenceValues = customEntitiesResult->GetValues();
    if (occurrenceRanges.empty()) {
        return;
    }

    for (int occurrenceIndex = 0; occurrenceIndex < occurrenceRanges.size(); ++occurrenceIndex) {
        const NProto::TRange& occurrenceRange = occurrenceRanges[occurrenceIndex];
        const NAlice::NNlu::TCustomEntityValues& values = occurrenceValues[occurrenceIndex];

        for (const NAlice::NNlu::TCustomEntityValue& value : values.GetCustomEntityValues()) {
            TString featureName = value.GetType();
            featureName.to_upper();
            const NNlu::TInterval convertedRange
                = alignment.GetMap1To2().ConvertInterval({occurrenceRange.GetBegin(), occurrenceRange.GetEnd()});
            if (convertedRange.Empty()) {
                continue;
            }
            AddFeatureSpan(featureName, convertedRange.Begin, convertedRange.End, nerFeatures);
        }
    }
}

void TFeatureExtractor::ExtractGeoAddrFeature(
    const NProto::TExternalMarkupProto& externalMarkup,
    const NNlu::TAlignment& alignment,
    TVector<TVector<NVins::TSparseFeature>>* features
) const {
    const auto& geoAddrResults = externalMarkup.GetGeoAddr();

    for (const NProto::TExternalMarkupProto::TGeoAddr& geoAddrResult : geoAddrResults) {
        for (const auto& field : geoAddrResult.GetFields()) {
            const TString featureName = "GeoAddr_" + field.GetType();
            const NProto::TExternalMarkupProto::TTokenSpan tokenSpan = field.GetTokens();

            const NNlu::TInterval interval = GetInterval(tokenSpan.GetBegin(), tokenSpan.GetEnd(), alignment);
            AddFeatureSpan(featureName, interval.Begin, interval.End, features);
        }
    }
}

void TFeatureExtractor::ExtractDateFeature(
    const NProto::TExternalMarkupProto& externalMarkup,
    const NNlu::TAlignment& alignment,
    TVector<TVector<NVins::TSparseFeature>>* features
) const {
    const auto& dateResults = externalMarkup.GetDate();

    for (const NProto::TExternalMarkupProto::TDate& dateResult : dateResults) {
        const TString featureName = dateResult.GetRelativeDay() ? "Date_RelativeDay" : "Date";
        const NProto::TExternalMarkupProto::TTokenSpan tokenSpan = dateResult.GetTokens();

        const NNlu::TInterval interval = GetInterval(tokenSpan.GetBegin(), tokenSpan.GetEnd(), alignment);
        AddFeatureSpan(featureName, interval.Begin, interval.End, features);
    }
}

void TFeatureExtractor::ExtractFioFeature(
    const NProto::TExternalMarkupProto& externalMarkup,
    const NNlu::TAlignment& alignment,
    TVector<TVector<NVins::TSparseFeature>>* features
) const {
    const auto& fioResults = externalMarkup.GetFio();

    for (const NProto::TExternalMarkupProto::TFio& fioResult : fioResults) {
        const TString featureName = "Fio_" + fioResult.GetType();
        const NProto::TExternalMarkupProto::TTokenSpan& tokenSpan = fioResult.GetTokens();

        const NNlu::TInterval interval = GetInterval(tokenSpan.GetBegin(), tokenSpan.GetEnd(), alignment);
        AddFeatureSpan(featureName, interval.Begin, interval.End, features);
    }
}

void TFeatureExtractor::ExtractIsNavFeature(
    const NProto::TIsNavResult* isNavResult,
    size_t tokensCount,
    TVector<TVector<NVins::TSparseFeature>>* features
) const {
    if (isNavResult && isNavResult->GetRuleResult() > 0) {
        AddFeatureSpan("IsNav", 0, tokensCount, features);
    }
}

void TFeatureExtractor::ExtractDirtyLangFeature(
    const NProto::TDirtyLangResult* dirtyLangResult,
    size_t tokensCount,
    TVector<TVector<NVins::TSparseFeature>>* features
) const {
    if (dirtyLangResult && dirtyLangResult->GetRuleResult() > 0) {
        AddFeatureSpan("DirtyLang", 0, tokensCount, features);
    }
}

static TVector<TEntityFinderParsedResult> CollectEntityFinderResults(
    const NProto::TEntityFinderResult* entityFinderResult
) {
    TVector<TEntityFinderParsedResult> results(Reserve(entityFinderResult->GetWinner().size()));

    for (const TString& winner : entityFinderResult->GetWinner()) {
        TVector<TString> fields;
        StringSplitter(winner).Split('\t').SkipEmpty().Collect(&fields);

        int begin = FromString(fields[1]);
        int end = FromString(fields[2]);
        double weight = FromString(fields[4]);
        TString ontoCategory = fields[5];
        TString freeBaseCategories = fields[6];

        results.push_back({begin, end, weight, ontoCategory, freeBaseCategories});
    }

    return results;
}

void TFeatureExtractor::ExtractEntityFinderFeature(
    const NProto::TEntityFinderResult* entityFinderResult,
    const NNlu::TAlignment& alignment,
    TVector<TVector<NVins::TSparseFeature>>* features
) const {
    if (!entityFinderResult) {
        return;
    }
    TVector<TEntityFinderParsedResult> results = CollectEntityFinderResults(entityFinderResult);

    Sort(
        results.begin(),
        results.end(),
        [](const auto& first, const auto& second) {
            return first.Begin < second.Begin
                || (first.Begin == second.Begin && first.End < second.End)
                || (first.Begin == second.Begin && first.End == second.End && first.Weight > second.Weight);
        }
    );

    for (size_t i = 0; i < results.size(); ++i) {
        if (i > 0 && results[i].Begin == results[i - 1].Begin && results[i].End == results[i - 1].End) {
            // For each token span we choose winner with highest score
            continue;
        }

        const NNlu::TInterval interval = GetInterval(results[i].Begin, results[i].End, alignment);

        AddFeatureSpan("EntityFinder_" + results[i].OntoCategory, interval.Begin, interval.End, features);

        TVector<TString> freeBaseCategories;
        StringSplitter(results[i].FreeBaseCategories).Split('|').SkipEmpty().Collect(&freeBaseCategories);
        for (const auto& freeBaseCategory : freeBaseCategories) {
            AddFeatureSpan("EntityFinder_" + freeBaseCategory, interval.Begin, interval.End, features);
        }
    }
}

void TFeatureExtractor::ExtractLocationAtHomeFeature(
    const NProto::TAliceCustomEntitiesResult* aliceCustomEntitiesResult,
    const NProto::TExternalMarkupProto& externalMarkup,
    const NJson::TJsonValue& nluExtra,
    const NNlu::TAlignment& alignment,
    TVector<TVector<NVins::TSparseFeature>>* features
) const {
    constexpr TStringBuf acceptedEntities[] = {"room", "group", "device", "multiroom_all_devices"};
    constexpr TStringBuf acceptedEntitiesInNluExtra[] = {"rooms", "groups", "devices", "multiroom_all_devices"};

    const TString text = JoinTokens(externalMarkup);
    for (const auto& [type, typeInInNluExtra] : Zip(acceptedEntities, acceptedEntitiesInNluExtra)) {
        for (const NJson::TJsonValue& item : nluExtra[typeInInNluExtra].GetArray()) {
            const TString& itemStr = item.GetString();
            if (const auto pos = text.find(itemStr); pos != TString::npos) {
                const size_t begin = std::count(std::begin(text), std::begin(text) + pos, ' ');
                const size_t end = begin + std::count(std::begin(itemStr), std::end(itemStr), ' ') + 1;
                const NNlu::TInterval interval = GetInterval(begin, end, alignment);
                AddFeatureSpan("Location_" + TString{type}, interval.Begin, interval.End, features);
            }
        }
    }

    if (aliceCustomEntitiesResult) {
        for (const auto& entity : aliceCustomEntitiesResult->GetEntities()) {
            TStringBuf type = entity.GetType();
            const bool typeStartsWithIoTPrefix = type.SkipPrefix(NGranet::NEntityTypePrefixes::IOT);
            if (typeStartsWithIoTPrefix && IsIn(acceptedEntities, type)) {
                AddFeatureSpan(TString("Location_") + type, entity.GetBegin(), entity.GetEnd(), features);
            }
        }
    }

}

static TString ConvertPosTagToVinsFormat(const TString& posTag, bool hasBrevGrammeme) {
    const auto vinsPosTagIter = POS_TAG_MAPPING.find(posTag);
    if (vinsPosTagIter == POS_TAG_MAPPING.end()) {
        return UNKNOWN_POS_TAG;
    }

    TString vinsPosTag = vinsPosTagIter->second;

    if (hasBrevGrammeme) {
        if (vinsPosTag == "ADJF") {
            vinsPosTag = "ADJS";
        } else if (vinsPosTag == "PRTF") {
            vinsPosTag = "PRTS";
        }
    }

    return vinsPosTag;
}

static TString ExtractPosTag(const NProto::TExternalMarkupProto::TMorphWord& morphoResult) {
    const auto& lemmas = morphoResult.GetLemmas();

    TString posTag = UNKNOWN_POS_TAG;
    bool hasBrevGrammeme = false;
    if (!lemmas.empty()) {
        const auto& grammemesList = lemmas[0].GetGrammems();
        if (!grammemesList.empty()) {
            TVector<TString> grammemes;
            StringSplitter(grammemesList[0]).Split(' ').SkipEmpty().Collect(&grammemes);
            posTag = grammemes[0];
            hasBrevGrammeme = AnyOf(grammemes, [](const auto& grammeme){ return grammeme == "brev"; });
        }
    }

    return ConvertPosTagToVinsFormat(posTag, hasBrevGrammeme);
}

TVector<TVector<NVins::TSparseFeature>> TFeatureExtractor::ExtractPosTagFeatures(
    const NProto::TExternalMarkupProto& externalMarkup,
    const NNlu::TAlignment& alignment,
    int tokenCount
) const {
    const auto& morphoAnalysisResults = externalMarkup.GetMorph();

    TVector<TVector<NVins::TSparseFeature>> postagFeatures(tokenCount);
    for (const NProto::TExternalMarkupProto::TMorphWord& morphoAnalysisResult : morphoAnalysisResults) {
        const TString posTag = ExtractPosTag(morphoAnalysisResult);
        const auto& tokenSpan = morphoAnalysisResult.GetTokens();
        const NNlu::TInterval interval = GetInterval(tokenSpan.GetBegin(), tokenSpan.GetEnd(), alignment);
        for (size_t tokenIndex = interval.Begin; tokenIndex < interval.End; ++tokenIndex) {
            postagFeatures[tokenIndex].emplace_back(posTag);
        }
    }

    for (auto& tokenFeatures : postagFeatures) {
        if (tokenFeatures.empty()) {
            tokenFeatures.emplace_back(UNKNOWN_POS_TAG);
        }
    }

    return postagFeatures;
}

TVector<TString> TFeatureExtractor::GetExternalMarkupTokens(const NProto::TExternalMarkupProto& markup) {
    TVector<TString> tokens;
    for (const NProto::TExternalMarkupProto::TToken& token : markup.GetTokens()) {
        tokens.push_back(token.GetText());
    }
    return tokens;
}

TVector<TString> TFeatureExtractor::GetNormalizedTokens(const NProto::TAliceNormalizerResult& normalizerResult) {
    return {std::begin(normalizerResult.GetNormalizedTokens()), std::end(normalizerResult.GetNormalizedTokens())};
}

NJson::TJsonValue TFeatureExtractor::DecodeNluExtra(const TString& encoded) {
    if (encoded.Empty()) {
        return {};
    }
    const TString base64decoded = Base64StrictDecode(encoded);
    if (base64decoded.Empty()) {
        return {};
    }
    return NAlice::JsonFromString(base64decoded);
}
