#include "fst_base.h"

#include <util/charset/unidata.h>
#include <util/charset/utf8.h>
#include <util/string/cast.h>
#include <util/string/split.h>
#include <util/string/strip.h>
#include <util/string/subst.h>

namespace NAlice {

    TString TFstBase::StripAndCollapse(TString string) {
        return StripInPlace(CollapseInPlace(string));
    }

    size_t TFstBase::CountWords(TStringBuf buf) {
        size_t count = 0u;
        auto current = reinterpret_cast<const unsigned char*>(buf.data());
        const auto end = current + buf.size();
        wchar32 symbol;
        RECODE_RESULT result;
        bool whitespace = true;
        while (current < end && (result = ReadUTF8CharAndAdvance(symbol, current, end)) == RECODE_RESULT::RECODE_OK) {
            if (IsWhitespace(symbol)) {
                whitespace = true;
                continue;
            } else if (whitespace == true) {
                ++count;
                whitespace = false;
            }
        }

        return count;
    }

    TString TFstBase::ReplaceSpecialSymbols(TString str, bool backwards) {
        constexpr TStringBuf specialTag = "<<SPECIAL_TAG_SYMBOL>>";
        constexpr TStringBuf specialTok = "<<SPECIAL_TOK_SYMBOL>>";

        if (backwards) {
            SubstGlobal(str, specialTag, TAG);
            SubstGlobal(str, specialTok, TOK);
        } else {
            SubstGlobal(str, TAG, specialTag);
            SubstGlobal(str, TOK, specialTok);
        }

        return str;
    }

    TFstBase::TFstBase(const TString& fstPath)
        : Decoder(fstPath)
    {
    }

    TFstBase::TFstBase(TFstDecoder&& decoder)
        : Decoder(std::move(decoder))
    {
    }

    TFstBase::TFstBase(const IDataLoader& loader)
        : Decoder(loader)
    {
    }

    TParsedToken TFstBase::ParseToken(TStringBuf token) const {
        TStringBuf type;
        TStringBuf stringValue;
        token.Split(TAG, type, stringValue);
        type = StripStringRight(type, EqualsStripAdapter('.'));
        TParsedToken parsedToken;
        parsedToken.StringValue = ReplaceSpecialSymbols(ToString(stringValue), true);
        parsedToken.Type = ToString(type);
        parsedToken.Value = ParseValue(parsedToken.Type, &parsedToken.StringValue, &parsedToken.Weight);
        parsedToken.Value = ProcessValue(parsedToken.Value);
        parsedToken.StringValue = StripAndCollapse(parsedToken.StringValue);

        return parsedToken;
    }

    TParsedToken::TValueType TFstBase::ParseValue(const TString& /*type*/, TString* stringValue, TMaybe<double>* /*weight*/) const {
        StripInPlace(*stringValue);
        int64_t value = 0;
        if (TryFromString<decltype(value)>(*stringValue, value)) {
            return value;
        }

        return TStringBuf{*stringValue};
    }

    TString TFstBase::PreNormalize(const TString& text) {
        if (text.Empty()) {
            return text;
        }
        auto current = reinterpret_cast<const unsigned char*>(text.c_str());
        auto end = current + text.size();
        TString preNormalized;
        preNormalized.reserve(text.size());
        preNormalized.push_back(' ');
        size_t offset = 1u;
        bool whitespacePending = false;
        wchar32 symbol;
        RECODE_RESULT result = RECODE_RESULT::RECODE_OK;
        while (current < end && (result = ReadUTF8CharAndAdvance(symbol, current, end)) == RECODE_RESULT::RECODE_OK) {
            if (IsWhitespace(symbol)) {
                whitespacePending = true;
                continue;
            } else if (whitespacePending == true) {
                whitespacePending = false;
                preNormalized.push_back(' ');
                preNormalized.push_back(' ');
                offset += 2u;
            }
            preNormalized.resize(offset + 4);
            size_t symbolLength = 0;
            WriteUTF8Char(symbol, symbolLength,
                const_cast<unsigned char*>(
                    reinterpret_cast<const unsigned char*>(preNormalized.c_str())) + offset);
            offset += symbolLength;
        }
        Y_ENSURE(result == RECODE_RESULT::RECODE_EOINPUT || result == RECODE_RESULT::RECODE_OK, "Failed to prenormalized string");
        preNormalized.resize(offset+1);
        preNormalized.back() = ' ';

        return preNormalized;
    }

    TString TFstBase::Decode(const TString& text) const {
        const auto& prenormalized = PreNormalize(text);
        return Decoder.Normalize(prenormalized);
    }

    TParsedToken::TValueType TFstBase::ProcessValue(const TParsedToken::TValueType& value) const {
        return value;
    }

    TVector<TEntity> TFstBase::Parse(const TString& utterance) const {
        TVector<TEntity> out;
        auto decoded = Decode(ToLowerUTF8(ReplaceSpecialSymbols(utterance)));
        size_t position = 0u;
        auto decodedBuf = TStringBuf{decoded};
        TStringBuf l, r;
        decodedBuf = decodedBuf.TrySplit(TOK, l, r) ? r : decodedBuf;
        if (decodedBuf.empty()) {
            return out;
        }
        for (const auto& token : StringSplitter(decodedBuf).SplitByString(TOK)) {
            auto&& parsedToken = ParseToken(token);
            const auto wordCount = CountWords(parsedToken.StringValue);
            out.push_back({position, position + wordCount, std::move(parsedToken)});
            position += wordCount;
        }

        return out;
    }

} // namespace NAlice
