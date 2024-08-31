#include "dump_lemmer.h"
#include <alice/nlu/granet/lib/utils/flag_utils.h>
#include <kernel/lemmer/dictlib/grambitset.h>
#include <kernel/lemmer/dictlib/tgrammar_processing.h>
#include <library/cpp/token/charfilter.h>
#include <util/generic/xrange.h>
#include <util/string/cast.h>

Y_DECLARE_OPERATORS_FOR_FLAGS(TFlags<TYandexLemma::TQuality>);

void DumpLemmer(TWtringBuf word, const TLangMask& langs, bool isVerbose, IOutputStream* log, const TString& indent) {
    Y_ENSURE(log);
    *log << indent << "============================================================" << Endl;
    *log << indent << "Word: " << word << Endl;
    *log << indent << "NLemmer::AnalyzeWord:" << Endl;
    TWLemmaArray lemmas;
    NLemmer::TAnalyzeWordOpt opt;
    // opt.GenerateQuasiBastards = ~TLangMask();
    // opt.GenerateAllBastards = true;
    NLemmer::AnalyzeWord(word.data(), word.length(), lemmas, langs, nullptr, opt);
    Dump(lemmas, isVerbose, log, indent + "  ");
}

void Dump(const TWLemmaArray& lemmas, bool isVerbose, IOutputStream* log, const TString& indent) {
    Y_ENSURE(log);
    *log << indent << "TWLemmaArray:" << Endl;
    for (const TYandexLemma& lemma : lemmas) {
        Dump(lemma, isVerbose, log, indent + "  ");
    }
    *log << indent << "Lemmas (brief):" << Endl;
    for (const TYandexLemma& lemma : lemmas) {
        *log << indent << "    " << TWtringBuf(lemma.GetText(), lemma.GetTextLength()) << Endl;
    }
}

static TString PrintGrams(const char* s) {
    if (s == nullptr) {
        return "nullptr";
    }
    return TGramBitSet::FromBytes(s).ToString();
}

void Dump(const TYandexLemma& lemma, bool isVerbose, IOutputStream* log, const TString& indent) {
    Y_ENSURE(log);
    *log << indent << "TYandexLemma:" << Endl;
    *log << indent << "  LemmaText: " << TWtringBuf(lemma.GetText(), lemma.GetTextLength()) << Endl;
    *log << indent << "  SuffixLen: " << lemma.GetSuffixLength() << Endl;
    *log << indent << "  Language: " << lemma.GetLanguage() << Endl;
    *log << indent << "  Quality: " << NGranet::FormatFlags(TFlags<TYandexLemma::TQuality>::FromBaseType(lemma.GetQuality())) << Endl;
    *log << indent << "  Depth: " << lemma.GetDepth() << Endl;
    *log << indent << "  StemGram: " << PrintGrams(lemma.GetStemGram()) << Endl;
    *log << indent << "  GramNum: " << lemma.FlexGramNum() << Endl;
    *log << indent << "  FlexGram:" << Endl;
    for (size_t i : xrange(lemma.FlexGramNum())) {
        *log << indent << "    " << PrintGrams(lemma.GetFlexGram()[i]) << Endl;
    }
    *log << indent << "  LemmaPrefLen: " << lemma.GetLemmaPrefLen() << Endl;
    *log << indent << "  PrefLen: " << lemma.GetPrefLen() << Endl;
    *log << indent << "  FlexLen: " << lemma.GetFlexLen() << Endl;
    *log << indent << "  FormInitial: " << TWtringBuf(lemma.GetInitialForm(), lemma.GetInitialFormLength()) << Endl;
    *log << indent << "  FormNormalized: " << TWtringBuf(lemma.GetNormalizedForm(), lemma.GetNormalizedFormLength()) << Endl;
    *log << indent << "  GetConvertedFormLength: " << lemma.GetConvertedFormLength() << Endl;
    *log << indent << "  Flags: " << Hex(lemma.GetCaseFlags()) << Endl;
    *log << indent << "  TokenPos: " << lemma.GetTokenPos() << Endl;
    *log << indent << "  TokenSpan: " << lemma.GetTokenSpan() << Endl;
    *log << indent << "  AdditionGram: " << PrintGrams(lemma.GetAddGram()) << Endl;
    *log << indent << "  Distortions: " << JoinSeq(", ", MakeIteratorRange(lemma.GetDistortions(), lemma.GetDistortions() + 5)) << ", ..." << Endl;
    *log << indent << "  ParadigmId: " << lemma.GetParadigmId() << Endl;
    *log << indent << "  MinDistortion: " << lemma.GetMinDistortion() << Endl;
    *log << indent << "  Weight: " << lemma.GetWeight() << Endl;
    *log << indent << "  LanguageVersion: " << lemma.GetLanguageVersion() << Endl;
    *log << indent << "  LooksLikeLemma: " << lemma.LooksLikeLemma() << Endl;
    *log << indent << "  GetFormsCount: " << lemma.GetFormsCount() << Endl;

    TAutoPtr<NLemmer::TFormGenerator> generator = lemma.Generator();
    if (generator != nullptr) {
        *log << indent << "  Forms:" << Endl;
        if (isVerbose) {
            TYandexWordform form;
            while (generator->GenerateNext(form)) {
                Dump(form, log, indent + "    ");
            }
        } else {
            *log << indent << "    Text            Lang  StemGram        FlexGram" << Endl;
            TYandexWordform form;
            while (generator->GenerateNext(form)) {
                TUtf16String text = form.GetText();
                text.resize(Max<size_t>(text.size(), 15), u' ');
                for (size_t i : xrange(form.FlexGramNum())) {
                    *log << indent << "    " << text;
                    *log << " " << RightPad(form.GetLanguage(), 5);
                    *log << " " << RightPad(PrintGrams(form.GetStemGram()), 14);
                    *log << "  " << PrintGrams(form.GetFlexGram()[i]) << Endl;
                }
            }
        }
    }
}

void Dump(const TYandexWordform& form, IOutputStream* log, const TString& indent) {
    Y_ENSURE(log);
    *log << indent << "TYandexWordform:" << Endl;
    *log << indent << "  Text: " << form.GetText() << Endl;
    *log << indent << "  Language: " << form.GetLanguage() << Endl;
    *log << indent << "  StemLen: " << form.GetStemLen() << Endl;
    *log << indent << "  PrefixLen: " << form.GetPrefixLen() << Endl;
    *log << indent << "  SuffixLen: " << form.GetSuffixLen() << Endl;
    *log << indent << "  StemGram: " << PrintGrams(form.GetStemGram()) << Endl;
    *log << indent << "  FlexGram:" << Endl;
    for (size_t i : xrange(form.FlexGramNum())) {
        *log << indent << "    " << PrintGrams(form.GetFlexGram()[i]) << Endl;
    }
    *log << indent << "  Weight: " << form.GetWeight() << Endl;
}
