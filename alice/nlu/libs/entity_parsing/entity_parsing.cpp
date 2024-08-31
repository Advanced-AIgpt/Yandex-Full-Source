#include "entity_parsing.h"

#include <alice/nlu/libs/occurrence_searcher/occurrence_searcher.h>

#include <alice/nlu/proto/entities/custom.pb.h>

#include <kernel/inflectorlib/phrase/simple/simple.h>
#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/core/lemmer.h>

#include <library/cpp/logger/global/global.h>

#include <util/charset/utf8.h>
#include <util/charset/wide.h>
#include <util/folder/path.h>
#include <util/generic/algorithm.h>
#include <util/generic/hash_set.h>
#include <util/generic/strbuf.h>
#include <util/stream/output.h>
#include <util/string/split.h>

namespace NAlice::NNlu {

template<>
void Update(TCustomEntityValues* value, const TCustomEntityValues& newValue) {
    Y_ASSERT(value);
    for (const auto& newEntityValue : newValue.GetCustomEntityValues()) {
        auto* entityValue = (*value).AddCustomEntityValues();
        (*entityValue) = newEntityValue;
    }
}

namespace {

const TVector<TString> GRAMMATICAL_CASES = {
    "nom", // им    // Casus
    "gen", // род
    "dat", // дат
    "acc", // вин
    "ins", // тв
    "abl", // пр
};

const TVector<TString> GRAMMATICAL_NUMBERS = {
    "sg", // ед    // Numerus
    "pl", // мн
};

const NInfl::TSimpleInflector INFLECTOR{"rus"};

void PutCases(const TString& phrase, TVector<TString>* phraseForms) {
    phraseForms->reserve(phraseForms->size() + GRAMMATICAL_CASES.size());
    for (const auto& inflectCase : GRAMMATICAL_CASES) {
        phraseForms->push_back(WideToUTF8(INFLECTOR.Inflect(UTF8ToWide(phrase), inflectCase)));
    }
}

void PutCasesCartesianNumbers(const TString& phrase, TVector<TString>* phraseForms) {
    phraseForms->reserve(phraseForms->size() + GRAMMATICAL_CASES.size() * GRAMMATICAL_NUMBERS.size());
    for (const auto& case_ : GRAMMATICAL_CASES) {
        for (const auto& inflectNumber : GRAMMATICAL_NUMBERS) {
            phraseForms->push_back(WideToUTF8(INFLECTOR.Inflect(UTF8ToWide(phrase), case_ + ", " + inflectNumber)));
        }
    }
}

bool IsSingle(const TString& s) {
    const TVector<TStringBuf> words = StringSplitter(s).Split(' ');
    bool singleFound = false;
    for (const TStringBuf word : words) {
        TWLemmaArray lemmas;
        NLemmer::AnalyzeWord(UTF8ToWide(word).c_str(), UTF8ToWide(word).size(), lemmas, TLangMask(LANG_RUS));
        for (const TYandexLemma& lemma : lemmas) {
            if (lemma.HasGram(EGrammar::gSingular)) {
                singleFound = true;
            } else if (lemma.HasGram(EGrammar::gPlural)) {
                return false;
            }
        }
    }

    return singleFound;
}

TVector<TEntityConfig::TValue> ReadValues(const NJson::TJsonValue& valuesJson) {
    TVector<TEntityConfig::TValue> values(Reserve(valuesJson.GetMap().size()));
    for (const auto& [value, samples] : valuesJson.GetMap()) {
        TVector<TString> sampleList(Reserve(samples.GetArray().size()));
        for (const auto& sample : samples.GetArray()) {
            sampleList.push_back(sample.GetString());
        }
        values.push_back({value, sampleList});
    }
    return values;
}

TCustomEntityValues BuildCustomEntityValues(const TString& type, const TString& value) {
    TCustomEntityValues values;
    auto* pbCustomEntityValues = values.MutableCustomEntityValues();
    auto* pbCustomEntityValue = pbCustomEntityValues->Add();
    pbCustomEntityValue->SetType(type);
    pbCustomEntityValue->SetValue(value);
    return values;
}
} // namespace

void ReadEntitiesFromJson(const NJson::TJsonValue& entitiesJson, TVector<TEntityConfig>* entities) {
    Y_ASSERT(entities);
    entities->reserve(entities->size() + entitiesJson.GetArray().size());
    for (const auto& entity : entitiesJson.GetArray()) {
        TEntityConfig newEntity;
        if(entity.Has("language")) {
            newEntity.Language = LanguageByNameOrDie(entity["language"].GetString());
        }
        newEntity.Name = entity["entity"].GetString();
        newEntity.Inflect = !entity.Has("inflect") || entity["inflect"].GetBoolean();
        newEntity.InflectNumbers = entity["inflect_numbers"].GetBoolean();
        newEntity.Values = ReadValues(entity["values"]);
        entities->push_back(newEntity);
        INFO_LOG << newEntity.Name << " found\n";
    }
}

TVector<TEntityString> MakeEntityStrings(const TMaybe<THashSet<TString>>& entityTypes,
                                                     const TVector<TEntityConfig>& entities) {
    TVector<TEntityString> entityStrings;

    for (const TEntityConfig& entity : entities) {
        if (entityTypes.Defined() && !entityTypes->contains(entity.Name)) {
            continue;
        }

        for (const auto& [value, samples] : entity.Values) {
            for (TString sample : samples) {
                sample = NormalizeString(entity.Language, sample);

                if (sample.empty()) {
                    continue;
                }

                TVector<TString> phraseForms = {sample};
                if (entity.Inflect) {
                    if (entity.InflectNumbers && IsSingle(sample)) {
                        PutCasesCartesianNumbers(sample, &phraseForms);
                    } else {
                        PutCases(sample, &phraseForms);
                    }
                }

                for (TString& form : phraseForms) {
                    entityStrings.push_back({
                        .Sample = form.empty() ? sample : form,
                        .Type = entity.Name,
                        .Value = value
                    });
                }
            }
        }
    }
    return entityStrings;
}

TBlob BuildOccurrenceSearcherData(const TVector<TEntityString>& entityStrings) {
    NAlice::NNlu::TOccurrenceSearcherDataBuilder<TCustomEntityValues> builder;
    for (const TEntityString& str : entityStrings) {
        builder.Add(str.Sample, BuildCustomEntityValues(str.Type, str.Value));
    }
    return builder.Build();
}

} // namespace NAlice::NNlu
