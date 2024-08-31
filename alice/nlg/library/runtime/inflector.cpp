#include "inflector.h"

#include <kernel/inflectorlib/fio/simple/simple_fio_infl.h>
#include <kernel/inflectorlib/phrase/simple/simple.h>
#include <kernel/inflectorlib/pluralize/pluralize.h>

#include <util/charset/wide.h>
#include <util/generic/hash_set.h>
#include <util/string/join.h>
#include <util/string/split.h>
#include <util/string/strip.h>

#include <cmath>

namespace NAlice::NNlg {

namespace NPrivate {

namespace {

const THashMap<TStringBuf, TStringBuf> GRAM_TRANSLATION_TABLE = {
    // GRAM_POS
    {TStringBuf("NOUN"), TStringBuf("S")},
    {TStringBuf("ADJF"), TStringBuf("A")},
    {TStringBuf("ADJS"), TStringBuf("A")},
    {TStringBuf("COMP"), TStringBuf("ADV")},
    {TStringBuf("VERB"), TStringBuf("V")},
    {TStringBuf("INFN"), TStringBuf("inf")},
    {TStringBuf("PRTF"), TStringBuf("partcp")},
    {TStringBuf("PRTS"), TStringBuf("partcp")},
    {TStringBuf("GRND"), TStringBuf("ger")},
    {TStringBuf("NUMR"), TStringBuf("NUM")},
    {TStringBuf("ADVB"), TStringBuf("ADV")},
    {TStringBuf("NPRO"), TStringBuf("SPRO")},
    {TStringBuf("PRED"), TStringBuf("ADVPRO")},
    {TStringBuf("PREP"), TStringBuf("PR")},
    {TStringBuf("CONJ"), TStringBuf("CONJ")},
    {TStringBuf("PRCL"), TStringBuf("PART")},
    {TStringBuf("INTJ"), TStringBuf("INTJ")},

    // GRAM_CASE
    {TStringBuf("nomn"), TStringBuf("nom")},
    {TStringBuf("gent"), TStringBuf("gen")},
    {TStringBuf("datv"), TStringBuf("dat")},
    {TStringBuf("accs"), TStringBuf("acc")},
    {TStringBuf("ablt"), TStringBuf("ins")},
    {TStringBuf("loct"), TStringBuf("abl")},
    {TStringBuf("voct"), TStringBuf("voc")},
    {TStringBuf("loc2"), TStringBuf("loc")},

    // GRAM_NUMBER
    {TStringBuf("sing"), TStringBuf("sg")},
    {TStringBuf("plur"), TStringBuf("pl")},

    // GRAM_GENDER
    {TStringBuf("masc"), TStringBuf("m")},
    {TStringBuf("femn"), TStringBuf("f")},
    {TStringBuf("neut"), TStringBuf("n")},
};

const TString GEN_PL_GRAMS{TStringBuf("gen,pl")};
const TString GEN_SG_GRAMS{TStringBuf("gen,sg")};

template <typename TInflectorImpl>
TString InflectImpl(const TInflectorImpl& impl, const TString& target, const TString& cases) {
    // TODO(a-square): teach inflectorlib to understand UTF-8 natively
    // TODO(a-square): teach inflectorlib to accept stringbufs
    const auto result = impl.Inflect(UTF8ToWide(StripString(TStringBuf{target})),
                                     NPrivate::NormalizeInflectionCases(cases));
    if (result) {
        return WideToUTF8(result);
    }
    return target;
}

}  // namespace

TString NormalizeInflectionCase(const TString& inflCase) {
    Y_ENSURE(!inflCase.Contains(','));
    if (const auto* ptr = GRAM_TRANSLATION_TABLE.FindPtr(inflCase)) {
        return TString{*ptr};
    }
    return inflCase;
}

TString NormalizeInflectionCases(const TString& cases) {
    THashSet<TStringBuf> translatedCases;

    bool translationNeeded = false;
    for (const TStringBuf token : StringSplitter(cases).Split(',').SkipEmpty()) {
        TStringBuf translatedToken = token;
        if (const auto* ptr = GRAM_TRANSLATION_TABLE.FindPtr(token)) {
            translatedToken = *ptr;
            translationNeeded = true;
        }
        translatedCases.insert(translatedToken);
    }

    if (translationNeeded) {
        return JoinSeq(TStringBuf(","), translatedCases);
    }

    return cases;
}

}  // namespace NPrivate

class TNormalInflector final : public IInflector {
public:
    TString Inflect(const TString& target, const TString& cases) const override {
        return NPrivate::InflectImpl(Impl, target, cases);
    }

private:
    NInfl::TSimpleInflector Impl{"rus"};
};

class TFioInflector final : public IInflector {
public:
    TString Inflect(const TString& target, const TString& cases) const override {
        return NPrivate::InflectImpl(Impl, target, cases);
    }

private:
    NFioInflector::TSimpleFioInflector Impl{::LANG_RUS};
};

IInflector::~IInflector() = default;

std::unique_ptr<IInflector> CreateNormalInflector() {
    return std::make_unique<TNormalInflector>();
}

std::unique_ptr<IInflector> CreateFioInflector() {
    return std::make_unique<TFioInflector>();
}

TString PluralizeWords(const IInflector& inflector,
                       const TString& text,
                       std::variant<double, ui64> number,
                       const TString& inflCase,
                       ELanguage lang) {
    ui64 intNumber = 0;
    if (std::holds_alternative<double>(number)) {
        const double doubleNumber = std::get<double>(number);
        Y_ENSURE(std::isfinite(doubleNumber));
        Y_ENSURE(doubleNumber >= 0);
        if (doubleNumber != std::trunc(doubleNumber)) {
            return inflector.Inflect(text, NPrivate::GEN_SG_GRAMS);
        }
        intNumber = doubleNumber;
    } else {
        intNumber = std::get<ui64>(number);
    }

    if (intNumber % 1000 == 0) {
        return inflector.Inflect(text, NPrivate::GEN_PL_GRAMS);
    }

    const auto normalizedCase = NPrivate::NormalizeInflectionCase(inflCase);
    if (intNumber % 10 == 1 && (intNumber < 10 || intNumber % 100 != 11)) {
        return inflector.Inflect(text, normalizedCase + ",sg");
    }

    if (normalizedCase == TStringBuf("nom") || normalizedCase == TStringBuf("acc")) {
        // TODO(a-square): teach Pluralize to understand UTF-8 directly
        return WideToUTF8(NInfl::Pluralize(UTF8ToWide(text), intNumber, lang));
    }

    return inflector.Inflect(text, normalizedCase + ",pl");
}

TString SingularizeWords(TStringBuf text, ui64 number, ELanguage lang) {
    // TODO(a-square): teach Singularize to understand UTF-8 directly
    return WideToUTF8(NInfl::Singularize(UTF8ToWide(text), number, lang));
}

}  // namespace NAlice::NNlg
