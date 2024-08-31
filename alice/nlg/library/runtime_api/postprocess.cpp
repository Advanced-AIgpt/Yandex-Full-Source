#include "exceptions.h"
#include "library/cpp/json/common/defs.h"
#include "postprocess.h"

#include <library/cpp/json/json_prettifier.h>
#include <library/cpp/json/json_writer.h>
#include <contrib/libs/re2/re2/re2.h>
#include <util/stream/str.h>
#include <util/string/ascii.h>
#include <util/string/escape.h>
#include <util/string/strip.h>

namespace NAlice::NNlg {

namespace {

inline constexpr ui32 PUNCT_BEGIN = 0x20;
inline constexpr ui32 PUNCT_END = 0x40;

re2::RE2 UNQUOTE_NEWLINES_REGEX(R"regex((\s*\\n\s*))regex");

inline constexpr const unsigned char RANGLE[] = "»";
static_assert(sizeof(RANGLE) == 3 && RANGLE[2] == '\0');

// check that one word is enough to fit the entire punctuation LUT
static_assert(PUNCT_END - PUNCT_BEGIN <= CHAR_BIT * sizeof(ui32));

template <unsigned char Char>
constexpr ui32 PunctBit() {
    static_assert(PUNCT_BEGIN <= Char && Char < PUNCT_END);
    return 1U << (Char - PUNCT_BEGIN);
}

inline constexpr ui32 PunctMask() {
    return PunctBit<'.'>() |
           PunctBit<','>() |
           PunctBit<':'>() |
           PunctBit<'!'>() |
           PunctBit<'?'>() |
           PunctBit<';'>();
}

//
// How many chars should dump from string when error occured
//
inline constexpr size_t DELTA_TEXT = 100;

class TJsonErrCallbacks : public NJson::TJsonCallbacks {
public:
    void OnError(size_t off, TStringBuf reason) override {
        Offset_ = off;
        Reason_ = reason;
    }
    TString GetSnippet(TStringBuf sourceText) {
        int offset1 = (Offset_ <= DELTA_TEXT) ? 0 : Offset_ - DELTA_TEXT;
        int offset2 = (Offset_ + DELTA_TEXT > sourceText.size()) ? sourceText.size() : Offset_ + DELTA_TEXT;

        TString result("Initial error: ");
        result.append(Reason_);
        result.append("; source: '");
        result.append(sourceText.SubString(offset1, offset2 - offset1));
        result.append("'");
        return result;
    }
private:
    size_t Offset_ = 0;
    TString Reason_;
};

}  // namespace

namespace NPrivate {

// we remove spaces right before *some* punctuation signs
bool IsSpecialPunct(TStringBuf str) {
    if (!str) {
        return false;
    }

    ui32 c = static_cast<unsigned char>(str[0]);
    if (PUNCT_BEGIN <= c && c < PUNCT_END) {
        // check for [.,:!?;] using a 32-bit LUT
        return PunctMask() & (1U << (c - PUNCT_BEGIN));
    }

    // check for "»" directly
    if (str.size() < 2) {
        return false;
    }
    return c == RANGLE[0] && static_cast<unsigned char>(str[1]) == RANGLE[1];
}

// equivalent to two passes in Python:
// s = re.sub(r'\s+((?=[.,:!?;»]))', '\\1', s, flags=re.UNICODE)
// s = re.sub(r'\s+', ' ', s, flags=re.UNICODE)
TString CollapsePunctWhitespace(TStringBuf str) {
    const char* begin = str.data();
    const char* end = begin + str.size();

    TString result;
    TStringOutput out(result);

    const char* cur = begin;
    while (begin != end) {
        while (cur != end && IsAsciiSpace(*cur)) {
            ++cur;
        }

        Y_ASSERT(cur <= end);

        if (!IsSpecialPunct(TStringBuf{cur, end})) {
            out << ' ';
        }

        begin = cur;
        while (cur != end && !IsAsciiSpace(*cur)) {
            ++cur;
        }

        Y_ASSERT(cur <= end);
        out << TStringBuf{begin, cur};
        begin = cur;
    }

    return result;
}

TString UnquoteNewlines(TStringBuf str) {
    TString result;
    TStringOutput out(result);

    re2::StringPiece input{str.data(), str.size()};

    while (!input.empty()) {
        re2::StringPiece newline;
        const char* segmentBegin = input.data();
        if (!re2::RE2::FindAndConsume(&input, UNQUOTE_NEWLINES_REGEX, &newline)) {
            out << TStringBuf{input.data(), input.size()};
            break;
        }

        out << TStringBuf{segmentBegin, newline.data()} << '\n';
    }

    return result;
}

TString PostprocessPhraseString(TStringBuf str) {
    // The process:
    // 1. remove whitespace before some punctuation signs
    // 2. collapse whitespace
    // 3. unquote newlines (and remove whitespace around them)
    // 4. strip the result

    // (1) and (2) can be done at the same time
    auto result = CollapsePunctWhitespace(str);

    // (3) and (4) cannot easily piggy back on top of other steps
    // TODO(a-square): figure out if it's worthwhile to do it anyway
    // if postprocessing turns out to be too slow
    result = UnquoteNewlines(result);
    StripInPlace(result);

    return result;
}

void StripStringsRecursively(NJson::TJsonValue& json) {
    if (json.IsString()) {
        json = Strip(json.GetString());
    } else if (json.IsArray()) {
        for (auto& value : json.GetArraySafe()) {
            StripStringsRecursively(value);
        }
    } else if (json.IsMap()) {
        for (auto& [key, value] : json.GetMapSafe()) {
            StripStringsRecursively(value);
        }
    }
}

}  // namespace NPrivate

NJson::TJsonValue PostprocessCard(const TText& text, const bool reduceWhitespace) {
    NJson::TJsonValue json;

    // Validate source text before parsing
    TJsonErrCallbacks errorDetails;
    TMemoryInput inputStream(text.GetBounds());

    ReadJson(&inputStream, &errorDetails);
    // If Json is not valid, next parsing will cause an error

    try {
        if (reduceWhitespace) {
            TString reducedText = NPrivate::PostprocessPhraseString(text.GetStr());
            NJson::ReadJsonTree(reducedText, &json, /* throwOnError = */ true);
            NPrivate::StripStringsRecursively(json);
        } else {
            TMemoryInput input(text.GetBounds());
            json = NJson::ReadJsonTree(&input, /* throwOnError = */ true);
        }
    } catch (const yexception& exc) {
        std::throw_with_nested(TCardValidationError() << exc.AsStrBuf() << ". " << errorDetails.GetSnippet(text.GetBounds()));
    }

    constexpr TStringBuf cardKey = "card";
    constexpr TStringBuf templatesKey = "templates";
    constexpr TStringBuf statesKey = "states";
    constexpr TStringBuf backgroundKey = "background";

    // Check that div2 cards don't have top-level div1 elements, and vice versa.
    // Div1 elements: see portal/morda-schema/div/1/div-data.json
    // Div2 elements: card and (optional) templates
    // - this is our invention, it's kind of like singularized Megamind response format
    //
    // XXX(a-square): what we're really doing here is a kind of a simplified version of
    // schema-based validation, we aren't doing the whole article here because we hope to
    // migrate away from doing cards using Jinja2 at all, and it would be too much work
    // to implement complete, possibly slow validation only to throw it away months later.
    if (json.Has(cardKey)) {
        // div2
        Y_ENSURE_EX(!json.Has(backgroundKey), TCardValidationError() << "Unexpected \"background\" in a div1 card");
        Y_ENSURE_EX(!json.Has(statesKey), TCardValidationError() << "Unexpected \"states\" in a div1 card");
    } else {
        // div1
        Y_ENSURE_EX(!json.Has(templatesKey), TCardValidationError() << "Unexpected \"templates\" in a div1 card");

        // .. states is the only required field per portal/morda-schema/div/1/div-data.json
        Y_ENSURE_EX(json.Has(statesKey), TCardValidationError() << "Expected \"states\" in a div1 card");
    }

    return json;
}

TPhraseOutput PostprocessPhrase(const TText& text) {
    return {
        NPrivate::PostprocessPhraseString(text.ExtractSpans(TText::EFlag::Text)),
        NPrivate::PostprocessPhraseString(text.ExtractSpans(TText::EFlag::Voice)),
    };
}

}  // namespace NAlice::NNlg

template <>
void Out<NAlice::NNlg::TPhraseOutput>(
    IOutputStream& out,
    TTypeTraits<NAlice::NNlg::TPhraseOutput>::TFuncParam phraseOutput)
{
    NJson::TJsonWriter writer(&out, /* formatOutput = */ true);
    writer.OpenMap();
    writer.Write("text", phraseOutput.Text);
    writer.Write("voice", phraseOutput.Voice);
    writer.CloseMap();
}
