#include "custom_entities.h"

#include <alice/nlu/libs/entity_searcher/entity_searcher.h>
#include <alice/nlu/libs/entity_searcher/entity_searcher_builder.h>
#include <alice/nlu/libs/entity_searcher/sample_graph.h>
#include <alice/nlu/libs/normalization/normalize.h>
#include <alice/nlu/libs/occurrence_searcher/occurrence_searcher.h>
#include <alice/nlu/libs/request_normalizer/request_normalizer.h>
#include <alice/nlu/libs/lemmatization/lemmatize.h>
#include <alice/nlu/libs/tokenization/tokenizer.h>

#include <alice/nlu/granet/lib/sample/entity_utils.h>

#include <alice/library/iot/demo_smart_home.h>
#include <alice/library/iot/preprocessor.h>

#include <library/cpp/iterator/enumerate.h>
#include <library/cpp/regex/pcre/regexp.h>
#include <library/cpp/scheme/scheme.h>

#include <util/string/builder.h>
#include <util/string/join.h>


namespace NAlice::NIot {

constexpr TStringBuf EXACT_MATCHING_PREFIX = "#";

namespace {


TString NormalizeToken(const TStringBuf token, ELanguage lang) {
    auto result = ::NAlice::NNlu::NormalizeString(lang, token);
    result = ::NNlu::NormalizeText(result, lang);
    return result;
}


void NormalizeTokens(TVector<TString>& tokens, ELanguage lang) {
    for (auto& token : tokens) {
        token = NormalizeToken(token, lang);
    }
}

TVector<TString> SplitIntoNormalizedTokens(const TStringBuf sample, ELanguage lang) {
    return ::NNlu::CreateTokenizer(::NNlu::ETokenizerType::WHITESPACE, sample, lang)->GetNormalizedTokens();
}

TVector<TString> MakeLemmatizedTokens(const TVector<TString>& tokens, ELanguage lang) {
    TVector<TString> result;

    for (const auto& token : tokens) {
        result.push_back(::NNlu::LemmatizeWordBest(token, lang));
    }

    return result;
}

TStringBuf NormalizeType(TStringBuf type) {
    type.SkipPrefix("demo.");
    return type;
}

auto* FindVariations(const ICategorizedVariations& variations, const TString& sampleText, const TStringBuf type) {
    return variations.GetVariations(NormalizeType(type)).FindPtr(sampleText);
}


void CollectSpellingVariations(const TString& sampleText, const TStringBuf type, ELanguage language, TVector<TString>& result) {
    if (auto* spellingVariations = FindVariations(*GetPreprocessor(language).SpellingVariations, sampleText, type)) {
        for (const auto& spellingVariation : *spellingVariations) {
            result.push_back(spellingVariation);
        }
    }
}

void CollectSynonyms(const TString& sampleText, const TStringBuf type, ELanguage language, TVector<TString>& result) {
    if (auto* synonyms = FindVariations(*GetPreprocessor(language).Synonyms, sampleText, type)) {
        for (const auto& synonym : *synonyms) {
            result.push_back(synonym);
        }
    }
}

void CollectSubSynonyms(const TVector<TString>& sampleTokens, const TStringBuf type, ELanguage language, TVector<TString>& result) {
    if (!IsIn(GetPreprocessor(language).TypesSuitableForSubSynonyms, NormalizeType(type))) {
        return;
    }

    const int subSynonymsMaxNumTokens = static_cast<int>(GetPreprocessor(language).SubSynonymsMaxNumTokens);
    for (int begin = 0; begin < sampleTokens.ysize(); ++begin) {
        for (int end = begin + 1;
             end <= sampleTokens.ysize() && (end - begin) <= Min(subSynonymsMaxNumTokens, sampleTokens.ysize() - 1);
             ++end)
        {
            const auto subSample = JoinRange(" ", sampleTokens.begin() + begin, sampleTokens.begin() + end);
            if (GetPreprocessor(language).SubSynonyms.contains(subSample)) {
                result.push_back(subSample);
            }
        }
    }
}

void CollectPositionalVariations(const TVector<TString>& sampleTokens, ELanguage language, TVector<TString>& result) {
    TString positionalVariationSample;

    if (sampleTokens.ysize() == 2) {
        positionalVariationSample = TStringBuilder() << sampleTokens[1] << " " << sampleTokens[0];
    } else if (sampleTokens.ysize() == 3 && GetPreprocessor(language).Prepositions.contains(sampleTokens[0])) {
        positionalVariationSample = TStringBuilder() << sampleTokens[2] << " " << sampleTokens[0] << " " << sampleTokens[1];
    } else if (sampleTokens.ysize() == 3 && GetPreprocessor(language).Prepositions.contains(sampleTokens[1])) {
        positionalVariationSample = TStringBuilder() << sampleTokens[1] << " " << sampleTokens[2] << " " << sampleTokens[0];
    }

    if (!positionalVariationSample.empty()) {
        result.push_back(positionalVariationSample);
    }
}

TVector<TString> ComputeBannedSubSynonyms(const TVector<TString>& samples, const TStringBuf type, ELanguage lang) {
    TVector<TString> samplesWithSpellingVariations = samples;
    for (const auto& sample : samples) {
        CollectSpellingVariations(sample, type, lang, samplesWithSpellingVariations);
    }

    TVector<TString> subSynonyms;
    for (const auto& sample : samplesWithSpellingVariations) {
        CollectSubSynonyms(SplitIntoNormalizedTokens(sample, lang), type, lang, subSynonyms);
    }

    THashMap<TString, int> subSynonymToNumberOfSamples;
    for (const auto& subSynonym : subSynonyms) {
        subSynonymToNumberOfSamples[subSynonym]++;
    }

    TVector<TString> result;
    for (const auto& [subSynonym, numberOfSamples] : subSynonymToNumberOfSamples) {
        if (numberOfSamples != 1) {
            result.push_back(subSynonym);
        }
    }

    return result;
}


class TEntitiesVariationsMaker {
public:
    TEntitiesVariationsMaker(const TStringBuf sample, const TStringBuf type, const TStringBuf value, ELanguage lang,
                             const TVector<TString>& bannedSubSynonyms = {})
        : Language_(lang)
        , Type_(type)
        , Value_(value)
        , BannedSybSynonyms_(bannedSubSynonyms)
    {
        if (!sample.empty()) {
            Samples_.emplace_back(sample, lang, LEMMA_QUALITY);
            GrowSamples();
        }
    }

    void AddEntitiesTo(TVector<NNlu::TEntityString>* result) {
        for (const auto& sample : Samples_) {
            if (sample.Text.StartsWith(EXACT_MATCHING_PREFIX)) {
                DoAddEntityTo(sample.Text.substr(EXACT_MATCHING_PREFIX.size()), sample.Quality + EXACT_BONUS, result);
            } else {
                DoAddEntityTo(sample.LemmatizedText, sample.Quality, result);
                DoAddEntityTo(sample.Text, sample.Quality + EXACT_BONUS, result);
            }
        }
    }

private:
    void DoAddEntityTo(const TString& sample, double quality, TVector<NNlu::TEntityString>* result) {
        result->push_back(NNlu::TEntityString{
            .Sample = sample,
            .Type = Type_,
            .Value = Value_,
            .Quality = quality
        });
    }

    void GrowSamples() {
        Y_ASSERT(Samples_.ysize() == 1);

        GrowSamplesWithSpellingVariations(LEMMA_QUALITY);

        GrowSamplesWithSubSynonyms(CLOSE_VARIATION_QUALITY);
        GrowSamplesWithPositionalVariations(CLOSE_VARIATION_QUALITY);

        GrowSamplesWithSynonyms();
        GrowSamplesWithSpellingVariations(SYNONYM_QUALITY);
        GrowSamplesWithSubSynonyms(SYNONYM_QUALITY);
        GrowSamplesWithPositionalVariations(SYNONYM_QUALITY);
    }

    void GrowSamplesWithSpellingVariations(double quality = LEMMA_QUALITY) {
        TVector<TString> newSamplesTexts;
        for (const auto& sample : Samples_) {
            CollectSpellingVariations(sample.Text, Type_, Language_, newSamplesTexts);
        }
        DoGrowSamplesWith(newSamplesTexts, quality);
    }

    void GrowSamplesWithSubSynonyms(double quality = CLOSE_VARIATION_QUALITY) {
        TVector<TString> newSamplesTexts;

        for (const auto& sample : Samples_) {
            CollectSubSynonyms(sample.Tokens, Type_, Language_, newSamplesTexts);
        }

        EraseIf(newSamplesTexts, [&](const auto& sample) {
            return IsIn(BannedSybSynonyms_, sample);
        });

        DoGrowSamplesWith(newSamplesTexts, quality);
    }

    void GrowSamplesWithPositionalVariations(double quality = CLOSE_VARIATION_QUALITY) {
        TVector<TString> newSamplesTexts;
        for (const auto& sample : Samples_) {
            CollectPositionalVariations(sample.Tokens, Language_, newSamplesTexts);
        }
        DoGrowSamplesWith(newSamplesTexts, quality);
    }

    void GrowSamplesWithSynonyms() {
        TVector<TString> newSamplesTexts;
        for (const auto& sample : Samples_) {
            CollectSynonyms(sample.Text, Type_, Language_, newSamplesTexts);
        }
        DoGrowSamplesWith(newSamplesTexts, SYNONYM_QUALITY);
    }

    void DoGrowSamplesWith(const TVector<TString>& texts, double quality) {
        for (auto text : texts) {
            Samples_.emplace_back(text, Language_, quality);
        }
    }

    struct TSample {
        TVector<TString> Tokens;
        TVector<TString> LemmatizedTokens;
        TString Text;
        TString LemmatizedText;
        double Quality;

        TSample(const TStringBuf text, ELanguage lang, double quality)
            : Tokens(SplitIntoNormalizedTokens(text, lang))
            , LemmatizedTokens(MakeLemmatizedTokens(Tokens, lang))
            , Text(JoinSeq(" ", Tokens))
            , LemmatizedText(JoinSeq(" ", LemmatizedTokens))
            , Quality(quality)
        {
        }
    };

private:
    const ELanguage Language_;
    const TString Type_;
    const TString Value_;
    const TVector<TString>& BannedSybSynonyms_;
    TVector<TSample> Samples_;
};


void FilterEntities(TVector<NNlu::TEntityString>* entities) {
    THashMap<std::pair<TString, TString>, double> typeSamplesToBestQuality;
    for (const auto& entity : *entities) {
        const auto typeSample = std::make_pair(entity.Type, entity.Sample);
        auto [it, emplaced] = typeSamplesToBestQuality.emplace(typeSample, entity.Quality);
        if (!emplaced) {
            it->second = Max(it->second, entity.Quality);
        }
    }

    TVector<NNlu::TEntityString> result;
    THashSet<std::tuple<TString, TString, TString, double>> seenTypeSampleValuesQualities;
    for (const auto& entity : *entities) {
        const auto typeSampleValueQuality = std::make_tuple(entity.Type, entity.Sample, entity.Value, entity.Quality);
        const auto typeSample = std::make_pair(entity.Type, entity.Sample);
        if (GetEntityQualityType(typeSamplesToBestQuality[typeSample]) == GetEntityQualityType(entity.Quality)) {
            const auto [it, inserted] = seenTypeSampleValuesQualities.insert(typeSampleValueQuality);
            if (inserted) {
                result.push_back(entity);
            }
        }
    }

    *entities = result;
}

void AddUserIoTPrefixToTypes(TVector<NGranet::TEntity>* entities) {
    for (auto& entity : *entities) {
        entity.Type = NGranet::NEntityTypePrefixes::IOT + entity.Type;
    }
}


class TIoTEntitiesBuilder {
public:
    TIoTEntitiesBuilder(const NAlice::TIoTUserInfo& ioTConfig, ELanguage lang, const TString& typePrefix = "")
        : IoTConfig_(ioTConfig)
        , Language_(lang)
        , TypePrefix_(typePrefix)
    {
    }

    void AddEntitiesTo(TVector<NNlu::TEntityString>* result) {
        Result_ = result;

        AddEntitiesWithAliases(IoTConfig_.GetDevices(), "device");
        AddEntitiesWithAliases(IoTConfig_.GetGroups(), "group");

        AddEntities(IoTConfig_.GetColors(), "arg_color");
        AddEntities(IoTConfig_.GetRooms(), "room");
        AddEntities(IoTConfig_.GetHouseholds(), "household");
        AddEntities(IoTConfig_.GetScenarios(), "scenario");

        AddEntitiesForScenarioTriggers();
        AddEntitiesForCapabilities();
    }

private:
    void AddEntitiesForCapabilities() {
        for (const auto& device : IoTConfig_.GetDevices()) {
            for (const auto& capability : device.GetCapabilities()) {
                if (capability.GetType() == TIoTUserInfo_TCapability_ECapabilityType_ColorSettingCapabilityType) {
                    for (const auto& scene : capability.GetColorSettingCapabilityParameters().GetColorSceneParameters().GetScenes()) {
                        const auto value = AssembleCapabilityValue("devices.capabilities.color_setting", "color_scene", scene.GetID());
                        DoAddEntities(scene.GetName(), "action_value", value, {});
                    }
                } else if (capability.GetType() == TIoTUserInfo_TCapability_ECapabilityType_CustomButtonCapabilityType) {
                    const auto& params = capability.GetCustomButtonCapabilityParameters();
                    for (const auto& name : params.GetInstanceNames()) {
                        const auto value = AssembleCapabilityValue("devices.capabilities.custom.button", params.GetInstance());
                        DoAddEntities(name, "instance", value, {});
                        DoAddEntities(name, "custom_button", params.GetInstance(), {});
                    }
                }
            }
        }
    }

    template <class TIoTObjects>
    void AddEntities(const TIoTObjects& ioTObjects, const TStringBuf type) {
        TVector<std::pair<TString, TString>> samplesWithValues;
        for (const auto& ioTObject : ioTObjects) {
            samplesWithValues.emplace_back(ioTObject.GetName(), ioTObject.GetId());
        }
        DoAddEntitiesForType(samplesWithValues, type);
    }

    template <class TIoTObjects>
    void AddEntitiesWithAliases(const TIoTObjects& ioTObjects, TString type) {
        TVector<std::pair<TString, TString>> samplesWithValues;
        for (const auto& ioTObject : ioTObjects) {
            samplesWithValues.emplace_back(ioTObject.GetName(), ioTObject.GetId());
        }
        for (const auto& ioTObject : ioTObjects) {
            for (const auto& alias : ioTObject.GetAliases()) {
                samplesWithValues.emplace_back(alias, ioTObject.GetId());
            }
        }
        DoAddEntitiesForType(samplesWithValues, type);
    }

    void AddEntitiesForScenarioTriggers() {
        TVector<std::pair<TString, TString>> samplesWithValues;
        for (const auto& scenario : IoTConfig_.GetScenarios()) {
            for (const auto& trigger : scenario.GetTriggers()) {
                if (trigger.GetType() == NAlice::TIoTUserInfo_TScenario_TTrigger_ETriggerType_VoiceScenarioTriggerType) {
                    samplesWithValues.emplace_back(trigger.GetVoiceTriggerPhrase(), scenario.GetId());
                }
            }
        }
        DoAddEntitiesForType(samplesWithValues, "triggered_scenario");
    }

    void DoAddEntitiesForType(const TVector<std::pair<TString, TString>>& samplesWithValues, const TStringBuf type) {
        TVector<TString> samplesTexts;
        for (const auto& [sample, value] : samplesWithValues) {
            samplesTexts.push_back(sample);
        }
        const auto bannedSubSynonyms = ComputeBannedSubSynonyms(samplesTexts, type, Language_);

        for (const auto& [sample, value] : samplesWithValues) {
            DoAddEntities(sample, type, value, bannedSubSynonyms);
        }
    }

    void DoAddEntities(const TStringBuf sample, const TStringBuf type, const TStringBuf value,
                       const TVector<TString>& bannedSubSynonyms) {
        TEntitiesVariationsMaker(
            NormalizeToken(sample, Language_),
            TypePrefix_ + type,
            value,
            Language_,
            bannedSubSynonyms
        ).AddEntitiesTo(Result_);
    }

    static TString AssembleCapabilityValue(const TStringBuf type, const TStringBuf instance, const TStringBuf value = "") {
        NSc::TValue result;
        result["type"] = type;
        result["instance"] = instance;
        if (!value.empty()) {
            result["value"] = value;
        }
        return result.ToJsonSafe();
    }

private:
    const NAlice::TIoTUserInfo& IoTConfig_;
    const ELanguage Language_;
    TVector<NNlu::TEntityString>* Result_;
    TString TypePrefix_;
};


class TStaticEntitiesBuilder {
public:
    explicit TStaticEntitiesBuilder(ELanguage lang)
        : Language_(lang)
    {
    }

    void AddEntitiesTo(TVector<NNlu::TEntityString>* result) {
        Result_ = result;

        AddBowFormsEntities();
        AddFormsEntities();
        AddExactFormsEntities();
    }

private:
    void AddBowFormsEntities() {
        for (const auto& [preType, tokens] : GetPreprocessor(Language_).TypeToBowIndexTokens) {
            const auto type = TStringBuilder() << "bow_" << preType;
            for (const auto& sample : tokens) {
                DoAddEntity(sample, type, sample, LEMMA_QUALITY);
            }
        }
    }

    void AddFormsEntities() {
        for (const auto& [type, entries] : GetPreprocessor(Language_).Entities.GetDict()) {
            for (const auto& entry : entries.GetArray()) {
                const auto value = ExtractValue(entry["value"]);
                const bool useSynonyms = entry["use_synonyms"].GetBool(false);
                for (const auto& form : entry["forms"].GetArray()) {
                    if (useSynonyms) {
                        const auto normalizedSample = NormalizeToken(form.ForceString(), Language_);
                        TEntitiesVariationsMaker(normalizedSample, type, value, Language_).AddEntitiesTo(Result_);
                    } else {
                        DoAddEntity(form.ForceString(), type, value, LEMMA_QUALITY);
                    }
                }
            }
        }
    }

    void AddExactFormsEntities() {
        for (const auto& [type, entries] : GetPreprocessor(Language_).Entities.GetDict()) {
            for (const auto& entry : entries.GetArray()) {
                const auto value = ExtractValue(entry["value"]);
                for (const auto& form : entry["exact_forms"].GetArray()) {
                    DoAddEntity(form.ForceString(), type, value, EXACT_QUALITY);
                }
            }
        }
    }

    static TString ExtractValue(const NSc::TValue& value) {
        TString result;
        if (value.IsNull()) {
            return "null";
        } else if (value.IsDict()) {
            return value.ToJson();
        }
        return value.ForceString();
    }

    void DoAddEntity(const TStringBuf sample, const TStringBuf type, const TStringBuf value, double quality) {
        Result_->push_back(NNlu::TEntityString{
            .Sample = NormalizeToken(sample, Language_),
            .Type = TString(type),
            .Value = TString(value),
            .Quality = quality
        });
    }

private:
    ELanguage Language_;
    TVector<NNlu::TEntityString>* Result_;
};


class TStaticAndDemoEntitiesCollector {
public:
    static void CollectDemoEntities(ELanguage language, TVector<NNlu::TEntityString>& result) {
        const auto& self = *Singleton<TStaticAndDemoEntitiesCollector>();
        self.DoCollectEntities(language, self.DemoEntitiesStrings_, result);
    }

    static void CollectStaticEntities(ELanguage language, TVector<NNlu::TEntityString>& result) {
        const auto& self = *Singleton<TStaticAndDemoEntitiesCollector>();
        self.DoCollectEntities(language, self.StaticEntitiesStrings_, result);
    }

private:
    TStaticAndDemoEntitiesCollector() {
        for (const auto language : SupportedLanguages_) {
            LoadDemoEntities(language);
            LoadStaticEntities(language);
        }
    }

    void DoCollectEntities(const ELanguage language, const THashMap<ELanguage, TVector<NNlu::TEntityString>>& source,
                           TVector<NNlu::TEntityString>& result) const
    {
        if (source.contains(language)) {
            for (const auto& entity : source.at(language)) {
                result.push_back(entity);
            }
        }
    }

    void LoadDemoEntities(const ELanguage language) {
        const auto& demoSmartHome = *GetDemoSmartHome(language);
        const TString demoTypePrefix = "demo.";
        auto& demoEntitiesForLanguage = DemoEntitiesStrings_[language];
        TIoTEntitiesBuilder(demoSmartHome, language, demoTypePrefix).AddEntitiesTo(&demoEntitiesForLanguage);
        FilterEntities(&demoEntitiesForLanguage);
    }

    void LoadStaticEntities(const ELanguage language) {
        auto& staticEntitiesForLanguage = StaticEntitiesStrings_[language];
        TStaticEntitiesBuilder(language).AddEntitiesTo(&staticEntitiesForLanguage);
        FilterEntities(&staticEntitiesForLanguage);
    }

private:
    THashMap<ELanguage, TVector<NNlu::TEntityString>> DemoEntitiesStrings_;
    THashMap<ELanguage, TVector<NNlu::TEntityString>> StaticEntitiesStrings_;

    static const TVector<ELanguage> SupportedLanguages_;

    Y_DECLARE_SINGLETON_FRIEND();
};

const TVector<ELanguage> TStaticAndDemoEntitiesCollector::SupportedLanguages_ = {LANG_RUS, LANG_ARA};

void FilterOutExactEntitiesWithNonIdealQuality(TVector<NGranet::TEntity>& entities) {
    EraseIf(entities, [](const NGranet::TEntity& entity) {
        return (
            IsExactEntityQuality(entity.Quality) &&
            entity.LogProbability < 0
        );
    });
}

void FilterNonExactEntitiesWhenExactOnesArePresent(TVector<NGranet::TEntity>& entities) {
    THashSet<std::tuple<TString, ::NNlu::TInterval, TString>> typesIntervalsValuesWithExactPresence;
    for (const auto& entity : entities) {
        if (IsExactEntityQuality(entity.Quality)) {
            typesIntervalsValuesWithExactPresence.emplace(entity.Type, entity.Interval, entity.Value);
        }
    }
    EraseIf(entities, [&](const auto& entity) {
        return (
            !IsExactEntityQuality(entity.Quality) &&
            typesIntervalsValuesWithExactPresence.contains(std::make_tuple(entity.Type, entity.Interval, entity.Value))
        );
    });
}

void FilterSameEntitiesWithDifferentProbabilities(TVector<NGranet::TEntity>& entities) {
    THashMap<std::tuple<TString, ::NNlu::TInterval, TString, double>, double> typeIntervalValueQualityToBestLogProb;
    for (const auto& entity : entities) {
        const auto typeIntervalValueQuality = std::make_tuple(entity.Type, entity.Interval, entity.Value, entity.Quality);
        const auto currentValue = typeIntervalValueQualityToBestLogProb.contains(typeIntervalValueQuality) ?
                                  typeIntervalValueQualityToBestLogProb[typeIntervalValueQuality] : entity.LogProbability;
        typeIntervalValueQualityToBestLogProb[typeIntervalValueQuality] = Max(currentValue, entity.LogProbability);
    }
    EraseIf(entities, [&](const auto& entity) {
        const auto typeIntervalValueQuality = std::make_tuple(entity.Type, entity.Interval, entity.Value, entity.Quality);
        return entity.LogProbability != typeIntervalValueQualityToBestLogProb[typeIntervalValueQuality];
    });
}

NNlu::TSampleGraph MakeSampleGraph(const TVector<TString>& tokens, ELanguage lang) {
    TVector<TString> lemmas;
    for (const auto& token : tokens) {
        lemmas.push_back(JoinSeq(',', ::NNlu::LemmatizeWord(token, lang, ::NNlu::ANY_LEMMA_THRESHOLD)));
    }

    return NNlu::TSampleGraph(tokens, lemmas, /* synonyms */ {});
}

// TODO(igor-darov) use fst_NUM instead (DIALOG-8607)
void AddNumEntities(const TVector<TString>& tokens, TVector<NGranet::TEntity>& result) {
    TRegExMatch re("^[0-9]+$");
    for (const auto& [i, token] : Enumerate(tokens)) {
        if (re.Match(token.c_str())) {
            result.push_back(NGranet::TEntity{
                .Interval = ::NNlu::TInterval{i, i + 1},
                .Type = "arg_num",
                .Value = token
            });
        }
    }
}

class TRawEntitiesMaker {
public:
    TRawEntitiesMaker(const TVector<NGranet::TEntity>& entities, const TVector<TString>& tokens)
        : Entities_(entities)
        , Tokens_(tokens)
    {
    }

    TVector<TRawEntity> Run() && {
        CollectGrouppedEntities();
        ExpandAndAddEachGrouppedEntityToResult();
        return std::move(Result_);
    }

private:
    void CollectGrouppedEntities() {
        for (const auto& entity : Entities_) {
            if (IsIoTEntity(entity)) {
                const auto type = entity.Type.substr(NGranet::NEntityTypePrefixes::IOT.size());
                const auto tbeq = std::make_tuple(type, entity.Interval.Begin, entity.Interval.End, entity.Quality);
                GrouppedEntities_[tbeq].push_back(entity.Value);
            }
        }
    }

    void ExpandAndAddEachGrouppedEntityToResult() {
        for (const auto& [typeBeginEndQuality, values] : GrouppedEntities_) {
            auto [type, begin, end, quality] = typeBeginEndQuality;

            const auto normalizedType = ToString(NormalizeType(type));
            const auto text = MakeText(begin, end);
            const auto extra = MakeExtra(quality);

            for (const auto& value : CollectValues(normalizedType, values)) {
                Result_.emplace_back(value, normalizedType, text, begin, end, extra);
            }
        }
    }

    TString MakeText(int begin, int end) const {
        return JoinRange(' ', Tokens_.begin() + begin, Tokens_.begin() + end);
    }

    NSc::TValue MakeExtra(double quality) const {
        NSc::TValue extra;
        if (GetEntityQualityType(quality) == ITEQ_SYNONYM) {
            extra["is_synonym"] = true;
        } else if (GetEntityQualityType(quality) == ITEQ_CLOSE_VARIATION) {
            extra["is_close_variation"] = true;
        }
        if (IsExactEntityQuality(quality)) {
            extra["is_exact"] = true;
        }
        return extra;
    }

    TVector<NSc::TValue> CollectValues(const TStringBuf type, const TVector<TString>& values) const {
        TVector<NSc::TValue> result;

        if (IsIn(EntityTypesWithGrouppedValues, type)) {
            NSc::TValue value;
            value.AppendAll(values);
            result.push_back(value);
        } else {
            for (const auto& valueStr : values) {
                auto value = NSc::TValue::FromJson(valueStr);
                if (value.IsNumber() && !IsIn(EntityTypesWithNumericalValues, type)) {
                    value = value.ForceString();
                }
                result.push_back(value);
            }
        }

        return result;
    }

    bool IsIoTEntity(const NGranet::TEntity& entity) const {
        return entity.Type.StartsWith(NGranet::NEntityTypePrefixes::IOT);
    }

private:
    const TVector<NGranet::TEntity>& Entities_;
    const TVector<TString>& Tokens_;

    // (type, begin, end, quality) to values
    THashMap<std::tuple<TString, int, int, double>, TVector<TString>> GrouppedEntities_;
    TVector<TRawEntity> Result_;

    static constexpr TStringBuf EntityTypesWithNumericalValues[] = {"arg_num"};
    static constexpr TStringBuf EntityTypesWithGrouppedValues[] = {"scenario", "triggered_scenario", "room",
                                                                   "group", "device", "household", "arg_color"};
};

}  // namespace


TVector<NNlu::TEntityString> ParseIoTConfig(const NAlice::TIoTUserInfo& ioTUserInfo, ELanguage lang) {
    TVector<NNlu::TEntityString> result;

    TIoTEntitiesBuilder(ioTUserInfo, lang).AddEntitiesTo(&result);
    FilterEntities(&result);
    TStaticAndDemoEntitiesCollector::CollectDemoEntities(lang, result);
    TStaticAndDemoEntitiesCollector::CollectStaticEntities(lang, result);

    return result;
}

TVector<NGranet::TEntity> FindIoTEntities(const TVector<NNlu::TEntityString>& ioTEntitiesStrings,
                                          TVector<TString> tokens,
                                          const ELanguage lang)
{
    NormalizeTokens(tokens, lang);
    const auto sampleGraph = MakeSampleGraph(tokens, lang);
    const auto entitiesSearcherData = NAlice::NNlu::TEntitySearcherDataBuilder().Build(ioTEntitiesStrings);
    auto foundEntities = NAlice::NNlu::TEntitySearcher(entitiesSearcherData).Search(sampleGraph);

    FilterOutExactEntitiesWithNonIdealQuality(foundEntities);
    FilterNonExactEntitiesWhenExactOnesArePresent(foundEntities);
    FilterSameEntitiesWithDifferentProbabilities(foundEntities);

    AddNumEntities(tokens, foundEntities);

    AddUserIoTPrefixToTypes(&foundEntities);

    return foundEntities;
}

TVector<TRawEntity> MakeRawEntities(const TVector<NGranet::TEntity>& entities, const TVector<TString>& tokens) {
    return TRawEntitiesMaker(entities, tokens).Run();
}

}  // namespace NAlice::NIot
