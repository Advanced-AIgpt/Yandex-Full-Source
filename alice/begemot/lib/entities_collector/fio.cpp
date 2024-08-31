#include "fio.h"
#include "entities_collector.h"
#include <alice/nlu/granet/lib/sample/entity_utils.h>
#include <alice/nlu/granet/lib/utils/utils.h>
#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/core/lemmer.h>
#include <library/cpp/scheme/scheme.h>

using namespace NNlu;
using namespace NGranet;

namespace NBg::NAliceEntityCollector {

static NGranet::TEntity MakePASkillsFio(const TInterval& interval, const NSc::TValue& value) {
    TEntity entity;
    entity.Interval = interval;
    entity.Type = NEntityTypes::PA_SKILLS_FIO;
    entity.Value = value.ToJson();
    entity.LogProbability = NEntityLogProbs::PA_SKILLS_FIO;
    return entity;
}

static bool IsValidFioWord(TWtringBuf word, ELanguage lang) {
    TWLemmaArray lemmas;
    NLemmer::AnalyzeWord(word.data(), word.length(), lemmas, lang);
    if (lemmas.empty()) {
        return true;
    }
    const TYandexLemma& lemma = lemmas[0];
    return lemma.GetQuality() == 0
        && !TGrammarVector(lemma.GetStemGram()).HasGram(gObscene);
}

TVector<NGranet::TEntity> CollectPASkillsFio(const NProto::TExternalMarkupProto& markup) {
    TVector<TEntity> entities;
    TDynBitMap coverage;

    for (const NProto::TExternalMarkupProto::TFio& fio : markup.GetFio()) {
        NSc::TValue value;
        if (fio.HasFirstName()) {
            value["first_name"] = fio.GetFirstName();
        }
        if (fio.HasLastName()) {
            value["last_name"] = fio.GetLastName();
        }
        if (fio.HasPatronymic()) {
            value["patronymic_name"] = fio.GetPatronymic();
        }
        if (value.IsNull()) {
            continue;
        }
        const TInterval interval = ToInterval(fio.GetTokens());
        entities.push_back(MakePASkillsFio(interval, value));
        coverage.Set(interval.Begin, interval.End);
    }

    // wizard Fio rule will work only if last name was specified
    // to match first and patronymic names we will process Morph rule output looking for "persn" and "patrn" grammems
    // that weren't previously matched

    TInterval entityInterval;
    NSc::TValue entityValue;

    for (const NProto::TExternalMarkupProto::TMorphWord& morph : markup.GetMorph()) {
        const TInterval wordInterval = ToInterval(morph.GetTokens());
        if (coverage.Get(wordInterval.Begin)) {
            continue;
        }
        if (morph.GetLemmas().empty()) {
            continue;
        }
        const NProto::TExternalMarkupProto::TMorphWord::TLemma& lemma = morph.GetLemmas()[0];
        TString fieldName;
        for (const TString& grammemSet : lemma.GetGrammems()) {
            if (grammemSet.Contains(TStringBuf("persn"))) {
                fieldName = "first_name";
            } else if (grammemSet.Contains(TStringBuf("patrn"))) {
                fieldName = "patronymic_name";
            } else if (grammemSet.Contains(TStringBuf("famn"))) {
                fieldName = "last_name";
            }
            if (!fieldName.empty()) {
                break;
            }
        }
        if (fieldName.empty()) {
            continue;
        }
        if (!IsValidFioWord(UTF8ToWide(lemma.GetText()), LanguageByName(lemma.GetLanguage()))) {
            continue;
        }
        if (!entityInterval.Empty() && (entityInterval.End != wordInterval.Begin || entityValue.Has(fieldName))) {
            entities.push_back(MakePASkillsFio(entityInterval, entityValue));
            entityInterval.SetEmpty();
            entityValue.Clear();
        }
        entityInterval |= wordInterval;
        entityValue[fieldName] = lemma.GetText();
    }

    if (!entityInterval.Empty()) {
        entities.push_back(MakePASkillsFio(entityInterval, entityValue));
    }
    return entities;
}

} // namespace NBg::NAliceEntityCollector
