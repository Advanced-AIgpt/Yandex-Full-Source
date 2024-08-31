#include "ar_normalize.h"

#include <contrib/libs/re2/re2/re2.h>
#include <contrib/libs/re2/re2/stringpiece.h>

#include <library/cpp/unicode/normalization/normalization.h>

#include <util/charset/wide.h>

namespace NNlu {

    namespace NImpl {

        namespace {

            const RE2 REGEX_DEDIAC = RE2("[\u0650\u064e\u064b\u0651\u0640\u064f\u064c\u064d\u0652\u0670]");
            const RE2 REGEX_ALEF_NORMALIZE = RE2("[\u0625\u0623\u0671\u0622]");
            const RE2 REGEX_TEH_MARBUTA_NORMALIZE = RE2("[\u0629]");
            constexpr wchar16 RIAL_SIGN_CHAR = L'\ufdfc';
            constexpr TStringBuf RIAL_SIGN_NORMALIZED = "\u0631\u064a\u0627\u0644";
            constexpr wchar16 BASMALA_LIGATURE_CHAR = L'\ufdfd';
            constexpr TStringBuf BASMALA_LIGATURE_NORMALIZED = "\u0628\u0633\u0645 \u0627\u0644\u0644\u0647 \u0627\u0644\u0631\u062d\u0645\u0646 \u0627\u0644\u0631\u062d\u064a\u0645";
            constexpr TStringBuf ALEF_LETTER = "\u0627";
            constexpr TStringBuf HEH_LETTER = "\u0647";

            TString RegexSub(const RE2& pattern, TStringBuf replacement, TStringBuf string) {
                TString result(string);
                RE2::GlobalReplace(&result, pattern, re2::StringPiece(replacement));
                return result;
            }

            TString FixUnicodeChars(TStringBuf text) {
                TString result;
                for (wchar16 chr : UTF8ToWide(text)) {
                    if (chr == RIAL_SIGN_CHAR) {
                        result += RIAL_SIGN_NORMALIZED;
                    } else if (chr == BASMALA_LIGATURE_CHAR) {
                        result += BASMALA_LIGATURE_NORMALIZED;
                    } else {
                        result += WideToUTF8(TUtf16String(chr));
                    }
                }
                return result;
            }

        } // namespace

        TString Dediac(TStringBuf text) {
            return RegexSub(REGEX_DEDIAC, "", text);
        }

        TString NormalizeUnicode(TStringBuf text) {
            TString fixedText = FixUnicodeChars(text);
            return WideToUTF8(Normalize<NUnicode::NFKC>(UTF8ToWide(fixedText)));
        }

        TString NormalizeAlef(TStringBuf text) {
            return RegexSub(REGEX_ALEF_NORMALIZE, ALEF_LETTER, text);
        }

        TString NormalizeTehMarbuta(TStringBuf text) {
            return RegexSub(REGEX_TEH_MARBUTA_NORMALIZE, HEH_LETTER, text);
        }

    } // namespace NImpl

    TString NormalizeArabicText(TStringBuf text) {
        return NImpl::NormalizeTehMarbuta(NImpl::NormalizeAlef(NImpl::Dediac(NImpl::NormalizeUnicode(text))));
    }

    TUtf16String NormalizeArabicText(TWtringBuf text) {
        return UTF8ToWide(NormalizeArabicText(WideToUTF8(text)));
    }

} // namespace NNlu
