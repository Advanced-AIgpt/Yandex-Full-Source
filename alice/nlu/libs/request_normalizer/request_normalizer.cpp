#include "request_normalizer.h"

#include <alice/nlu/libs/fst/resource_data_loader.h>
#include <alice/nlu/libs/normalization/normalize.h>

#include <dict/dictutil/dictutil.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/string/join.h>
#include <util/string/split.h>
#include <util/string/strip.h>
#include <util/string/subst.h>

namespace NNlu {

static const THashMap<TString, TString> firstWordsExact = {
    {"дважды", "2 *"},
    {"трижды", "3 *"},
    {"четырежды", "4 *"},
    {"пятью", "5 *"},
    {"шестью", "6 *"},
    {"восемью", "8 *"},
    {"семью", "7 *"},
    {"девятью", "9 *"},
    {"десятью", "10 *"},
};
static const THashSet<TString> secondWordsExact = {
    "десять",
    "пятнадцать",
    "шестнадцать",
    "семнадцать",
    "восемнадцать",
    "девятнадцать",
    "сорок",
    "девяносто",
    "сто",
    "тысяча",
    "миллион",
    "миллиард",
};
static const THashSet<TString> secondWordsPrefix = {
    "один",
    "два",
    "две",
    "три",
    "четыр",
    "пять",
    "шесть",
    "семь",
    "восемь",
    "девять",
};

TRequestNormalizer::TRequestNormalizer()
{
    AddDenormalizer(LANG_RUS, "ru");
    AddDenormalizer(LANG_TUR, "tr");
}

void TRequestNormalizer::AddDenormalizer(ELanguage lang, TStringBuf langDir) {
    const TFsPath dir = TFsPath("/nlu/request_fst_normalizer/denormalizer") / langDir;

    // Use Singleton here because there were attempts to use TRequestNormalizer
    // while initialization of 'classic' static global variables.
    const TVector<TString>* blackList = Singleton<TVector<TString>>(TVector<TString>{
        "reverse_conversion.profanity",
        "reverse_conversion.make_substitution_group",
        "number.convert_size",
        "units.converter",
        "reverse_conversion.times",
        "reverse_conversion.remove_space_at_start"
    });

    Denormalizers.emplace(lang, NAlice::TFstDecoder(NAlice::TResourceDataLoader(dir), *blackList));
}

TString TRequestNormalizer::Normalize(ELanguage lang, TStringBuf text) {
    return Singleton<TRequestNormalizer>()->DoNormalize(lang, text);
}

TString TRequestNormalizer::DoNormalize(ELanguage lang, TStringBuf original) const {
    TString text = WideToUTF8(ToLower(lang, UTF8ToWide<true>(original)));
    if (lang == LANG_ARA) {
        return NormalizeText(text, lang);
    }
    const NAlice::TFstDecoder* denormalizer = Denormalizers.FindPtr(lang);
    if (!denormalizer) {
        return text;
    }
    text = JoinSeq(" ", TRequestTokenizer::Tokenize(text));
    // Normalizer bug: replace 'ε' symbol (special symbol in OpenFST)
    SubstGlobal(text, TStringBuf("ε"), TStringBuf("e"));

    text = Preprocess(text);
    text = StripString(denormalizer->Normalize(text));
    text = WideToUTF8(ToLower(lang, UTF8ToWide(text)));
    text = Postprocess(text);
    return text;
}

// Perform calc workaround fixes.
TString TRequestNormalizer::Preprocess(TString text) {
    TVector<TString> res;
    Split(text, " ", res);
    for (int i = 0, n = res.size(); i < n-1; i++) {
        if (firstWordsExact.contains(res[i])) {
            if (secondWordsExact.contains(res[i+1])) {
                res[i] = firstWordsExact.at(res[i]);
            } else if (AnyOf(secondWordsPrefix, [res, i](const auto& prefix) { return res[i+1].StartsWith(prefix); })) {
                res[i] = firstWordsExact.at(res[i]);
            }
        }
    }
    return JoinSeq(" ", res);
}

// Perform some workaround fixes.
// TODO(samoylovboris) Move that fixes to FST.
// static
TString TRequestNormalizer::Postprocess(TString text) {
    text = TString::Join(' ', text, ' ');

    // "2.5 1000 " -> "2500 "
    SubstGlobal(text, TStringBuf(",5 1000 "), TStringBuf("500 "));
    SubstGlobal(text, TStringBuf(",5 1000000 "), TStringBuf("500000 "));
    SubstGlobal(text, TStringBuf(" полторы 1000 "), TStringBuf(" 1500 "));
    SubstGlobal(text, TStringBuf(" полтора 1000000 "), TStringBuf(" 1500000 "));

    return StripString(text);
}

void TRequestNormalizer::WarmUpSingleton() {
    Normalize(LANG_RUS, "");
}

} // namespace NNlu
