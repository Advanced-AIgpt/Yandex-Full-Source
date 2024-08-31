#include "lemmatize.h"

#include <alice/nlu/libs/normalization/normalize.h>
#include <dict/nerutil/tstimer.h>
#include <kernel/lemmer/core/language.h>

#include <library/cpp/iterator/enumerate.h>

#include <util/generic/is_in.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NNlu {

    namespace {

        inline TWtringBuf LemmaWtringBuf(const TYandexLemma& lemma) {
            return TWtringBuf(lemma.GetText(), lemma.GetTextLength());
        }

        constexpr TWtringBuf SURNAME_LEMMA_FEMALE_TO_MALE_SUFFIX = u"а";
        constexpr TWtringBuf SURNAME_LEMMA_SURE_SUFFIXES[] = {
            u"ов", u"ев", u"ёв", u"ин", u"ын", u"их", u"ых", u"ский", u"цкий", // Russian
            u"ко", u"ич", // Ukrainian and Belarusian
        };

        bool IsLikeSurnameLemma(TWtringBuf word) {
            word.ChopSuffix(SURNAME_LEMMA_FEMALE_TO_MALE_SUFFIX);
            for (const TWtringBuf& suffix : SURNAME_LEMMA_SURE_SUFFIXES) {
                if (word.EndsWith(suffix)) {
                    return true;
                }
            }
            return false;
        }

        TWLemmaArray GenerateLemmasWithThreshold(TWtringBuf original, TWtringBuf normalized, ELanguage lang, double threshold) {
            TWLemmaArray result;
            for (auto [i, lemma] : Enumerate(GenerateLemmas(original, lang))) {
                if (i == 0
                    || lemma.GetWeight() >= threshold
                    || LemmaWtringBuf(lemma) == normalized
                    || lemma.HasGram(gFirstName)
                    || lemma.HasGram(gSurname)
                    || lemma.HasGram(gPatr))
                {
                    result.push_back(std::move(lemma));
                }
            }

            return result;
        }
    } // namespace

    TWLemmaArray GenerateLemmas(TWtringBuf word, ELanguage lang) {
        DEBUG_TIMER("GenerateLemmas");
        TWLemmaArray lemmas;
        NLemmer::AnalyzeWord(word.data(), word.length(), lemmas, lang);
        return lemmas;
    }

    TWLemmaArray GenerateLemmasWithThreshold(TWtringBuf word, ELanguage lang, double threshold) {
        return GenerateLemmasWithThreshold(word, NormalizeWord(word, lang), lang, threshold);
    }

    TVector<TUtf16String> LemmatizeWord(TWtringBuf original, ELanguage lang, double threshold) {
        const TUtf16String normalized = NormalizeWord(original, lang);
        TVector<TUtf16String> result;
        for (const auto& lemma : GenerateLemmasWithThreshold(original, normalized, lang, threshold)) {
            const TWtringBuf lemmaStr = LemmaWtringBuf(lemma);
            if (!IsIn(result, lemmaStr)) {
                result.push_back(ToWtring(lemmaStr));
            }
        }

        if (result.empty()) {
            result.push_back(normalized);
        } else if (!IsIn(result, normalized) && IsLikeSurnameLemma(normalized)) {
            result.push_back(normalized);
        }

        return result;
    }

    TVector<TString> LemmatizeWord(TStringBuf original, ELanguage lang, double threshold) {
        const TVector<TUtf16String> resultWide = LemmatizeWord(UTF8ToWide(original), lang, threshold);
        TVector<TString> result(Reserve(resultWide.size()));
        for (const auto& lemmaWide : resultWide) {
            result.push_back(WideToUTF8(lemmaWide));
        }

        return result;
    }

    TUtf16String LemmatizeWordBest(TWtringBuf word, ELanguage lang) {
        const TWLemmaArray lemmas = GenerateLemmas(word, lang);
        if (lemmas.empty()) {
            return NormalizeWord(word, lang);
        }
        return ToWtring(LemmaWtringBuf(lemmas.front()));
    }

    TString LemmatizeWordBest(TStringBuf word, ELanguage lang) {
        return WideToUTF8(LemmatizeWordBest(UTF8ToWide(word), lang));
    }

} // namespace NNlu
