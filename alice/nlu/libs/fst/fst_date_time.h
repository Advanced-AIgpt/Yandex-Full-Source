#pragma once

#include "fst_base_value.h"

#include <re2/re2.h>

namespace NAlice {

    class TFstDateTime : public TFstBaseValue {
    public:
        using TFstBaseValue::TFstBaseValue;

        static TParsedToken::TValueType ApplyRelOut(TMap<char, int64_t> out, TMultiMap<char, int64_t> wholeRelOut);

    protected:
        TVector<TStringBuf> FindValues(const TStringBuf& tape) const;
        virtual TParsedToken::TValueType ToDateTime(const TVector<TStringBuf>& values) const;
        TParsedToken::TValueType ParseValue(
            const TString& type, TString* stringValue, TMaybe<double>* weight) const override;
        virtual const RE2& TapePattern() const;
    };

} // namespace NAlice
