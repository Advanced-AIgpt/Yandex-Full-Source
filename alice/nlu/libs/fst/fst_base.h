#pragma once

#include "data_loader.h"
#include "decoder.h"

#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/variant.h>
#include <util/generic/vector.h>

#include <library/cpp/scheme/scheme.h>

namespace NAlice {

    struct TParsedToken {
        using TValueType = NSc::TValue;
        TString StringValue;
        TString Type;
        TMaybe<double> Weight;
        TValueType Value;
    };

    struct TEntity {
        size_t Start;
        size_t End;
        TParsedToken ParsedToken;
    };

    class TFstBase {
    public:
        static TString PreNormalize(const TString& text);

        virtual ~TFstBase() = default;

        explicit TFstBase(const TString& fstPath);
        explicit TFstBase(const IDataLoader& loader);
        explicit TFstBase(TFstDecoder&& decoder);

        TVector<TEntity> Parse(const TString& utterance) const;

        TParsedToken ParseToken(TStringBuf token) const;

        virtual TParsedToken::TValueType ParseValue(const TString& type, TString* stringValue, TMaybe<double>* weight) const;
        virtual TParsedToken::TValueType ProcessValue(const TParsedToken::TValueType& value) const;

        static TString StripAndCollapse(TString string);
        static size_t CountWords(TStringBuf buf);
        static TString ReplaceSpecialSymbols(TString str, bool backwards = false);

    public:
        static constexpr char TAG[] = "#";
        static constexpr char TOK[] = "|";

    private:
        TString Decode(const TString& text) const;

    private:
        TFstDecoder Decoder;
    };

} // namespace NAlice
