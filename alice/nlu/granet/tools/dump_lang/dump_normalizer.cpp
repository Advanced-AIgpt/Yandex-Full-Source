#include "dump_normalizer.h"
#include <alice/nlu/granet/lib/utils/flag_utils.h>
#include <alice/nlu/libs/request_normalizer/request_normalizer.h>
#include <alice/nlu/libs/normalization/normalize.h>
#include <alice/nlu/libs/tokenization/tokenizer.h>
#include <dict/dictutil/dictutil.h>
#include <kernel/lemmer/dictlib/grambitset.h>
#include <kernel/lemmer/dictlib/tgrammar_processing.h>
#include <library/cpp/token/charfilter.h>
#include <util/generic/xrange.h>
#include <util/string/cast.h>

static IOutputStream& PrintKey(IOutputStream* log, const TString& indent, TStringBuf str) {
    *log << indent << RightPad(str, 32);
    return *log;
}

void DumpNormalizer(TWtringBuf lineBuf, const TLangMask& langs, IOutputStream* log, const TString& indent) {
    Y_ENSURE(log);
    const TUtf16String line(lineBuf);
    *log << indent << "============================================================" << Endl;
    PrintKey(log, indent, "Original:") << line << Endl;
    PrintKey(log, indent, "NormalizeUnicode:") <<NormalizeUnicode(line) << Endl;
    for (const ELanguage lang : langs) {
        *log << indent << NameByLanguage(lang) << Endl;
        PrintKey(log, indent, "  ToLower:") << ToLower(lang, line) << Endl;
        PrintKey(log, indent, "  RemoveDiacritics:") << RemoveDiacritics(lang, line) << Endl;
        PrintKey(log, indent, "  NGranet::NormalizeText:") << NNlu::NormalizeText(line, lang) << Endl;
        PrintKey(log, indent, "  NGranet::TTokenizer:")
            << JoinSeq(" ", NNlu::TSmartTokenizer(WideToUTF8(line), lang).GetNormalizedTokens()) << Endl;
        PrintKey(log, indent, "  NNlu::TRequestNormalizer:")
            << NNlu::TRequestNormalizer::Normalize(lang, WideToUTF8(line)) << Endl;
    }
}
