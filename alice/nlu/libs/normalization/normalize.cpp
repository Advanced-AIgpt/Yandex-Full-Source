#include "normalize.h"
#include "ar_normalize.h"

#include <dict/dictutil/dictutil.h>

#include <util/charset/unidata.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/string/cast.h>
#include <util/string/join.h>
#include <util/string/split.h>
#include <util/string/subst.h>
#include <util/string/util.h>


namespace NNlu {

    namespace {

        TUtf16String NormalizeTokens(const TWtringBuf text, ELanguage lang) {
            if (lang == LANG_RUS) {
                TVector<TString> tokens = StringSplitter(WideToUTF8(text)).SplitByFunc(IsWhitespace).SkipEmpty();
                for (const auto& [replaceWhat, replaceWith] : TVector<std::pair<TStringBuf, TStringBuf>>{
                    {"%", "процент"},
                    {"°", "градус"},
                }) {
                    std::replace(tokens.begin(), tokens.end(), replaceWhat, replaceWith);
                }

                return UTF8ToWide(JoinSeq(" ", tokens));
            }
            return ToWtring(text);
        }
    }

    TUtf16String NormalizeText(TWtringBuf text, ELanguage lang) {
        TUtf16String result = ToLower(lang, text);

        SubstGlobal(result, u'ё', u'е');
        result = NormalizeTokens(result, lang);

        if (lang == LANG_ARA) {
            return NormalizeArabicText(result);
        }
        return result;
    }

    TString NormalizeText(TStringBuf text, ELanguage lang) {
        return WideToUTF8(NormalizeText(UTF8ToWide(text), lang));
    }

    TUtf16String NormalizeWord(TWtringBuf word, ELanguage lang) {
        TUtf16String result = NormalizeText(word, lang);
        RemoveAll(result, u' ');
        return result;
    }

    TString NormalizeWord(TStringBuf word, ELanguage lang) {
        return WideToUTF8(NormalizeWord(UTF8ToWide(word), lang));
    }

} // namespace NNlu
