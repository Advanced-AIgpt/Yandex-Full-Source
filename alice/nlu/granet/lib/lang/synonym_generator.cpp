#include "synonym_generator.h"

#include <alice/nlu/libs/lemmatization/lemmatize.h>
#include <kernel/inflectorlib/phrase/complexword.h>
#include <kernel/inflectorlib/phrase/gramfeatures.h>
#include <kernel/inflectorlib/phrase/simple/simple.h>
#include <kernel/inflectorlib/phrase/ylemma.h>
#include <kernel/lemmer/core/language.h>
#include <library/cpp/langmask/langmask.h>
#include <util/charset/wide.h>
#include <util/stream/str.h>
#include <util/string/strip.h>
#include <util/string/subst.h>

namespace NGranet {

namespace {

    struct TInflectorTransformation {
        TGramBitSet Patch;
        TString Template;
    };

    struct TInflectorRule {
        TGramBitSet Condition;
        TVector<TInflectorTransformation> Transformations;
    };

    const TInflectorRule RULES[] = {
        {
            TGramBitSet(gVerb, gImperative, gPerson2, gPerfect), // включи
            {
                {TGramBitSet(gSingular),                "$"}, // включи
                {TGramBitSet(gPlural),                  "$"}, // включите
                {TGramBitSet(gInfinitive),              "$"}, // включить
                {TGramBitSet(gInfinitive),              "можешь $"}, // можешь включить
                {TGramBitSet(gImperfect, gSingular),    "$"}, // включай
                {TGramBitSet(gImperfect, gPlural),      "$"}, // включайте
            }
        },
        {
            TGramBitSet(gVerb, gImperative, gPerson2, gImperfect), // включай
            {
                {TGramBitSet(gSingular),                "$"}, // включай
                {TGramBitSet(gPlural),                  "$"}, // включайте
                {TGramBitSet(gInfinitive),              "$"}, // включать
            }
        },
    };

    TString Inflect(const TYandexLemma& lemma, size_t flexIndex, const TGramBitSet& destGram) {
        NInfl::TYemmaInflector inflector(lemma, flexIndex);
        if (!inflector.Inflect(destGram, false)) {
            return {};
        }
        TUtf16String result;
        inflector.ConstructText(result);
        return WideToUTF8(result);
    }

    TVector<TStringWithWeight> DoGenerateBuiltinSynonyms(TStringBuf original) {
        TVector<TString> synonyms;
        for (const TYandexLemma& lemma : NNlu::GenerateLemmas(UTF8ToWide(original), LANG_RUS)) {
            for (size_t flexIndex = 0; flexIndex < lemma.FlexGramNum(); ++flexIndex) {
                TGramBitSet srcGram;
                NSpike::ToGramBitset(lemma.GetStemGram(), lemma.GetFlexGram()[flexIndex], srcGram);
                for (const TInflectorRule& rule : RULES) {
                    if (!srcGram.HasAll(rule.Condition)) {
                        continue;
                    }
                    for (const TInflectorTransformation& transformation : rule.Transformations) {
                        const TGramBitSet destGram = NInfl::DefaultFeatures().ReplaceFeatures(srcGram, transformation.Patch);
                        const TString inflected = Inflect(lemma, flexIndex, destGram);
                        if (inflected.empty()) {
                            continue;
                        }
                        TString phrase = transformation.Template;
                        SubstGlobal(phrase, TStringBuf("$"), inflected);
                        synonyms.push_back(phrase);
                    }
                }
            }
        }
        TVector<TStringWithWeight> result;
        result.push_back({TString{original}, 1.});
        for (const TString& synonym : synonyms) {
            result.push_back({synonym, 1. / synonyms.size()});
        }
        SortAndMerge(&result);
        NormalizeWeights(&result);
        return result;
    }

} // namespace

TVector<TStringWithWeight> GenerateBuiltinSynonyms(TStringBuf original) {
    return DoGenerateBuiltinSynonyms(original);
}

} // namespace NGranet
