#pragma once

#include "fst_base.h"

namespace NAlice {

    class TFstBaseValue : public TFstBase {
    public:
        using TFstBase::TFstBase;
        TParsedToken::TValueType ParseValue(
            const TString& type, TString* stringValue, TMaybe<double>* weight) const override;

    public:
        static constexpr char V_BEG = '{';
        static constexpr char V_END = '}';
        static constexpr char V_INSERTED = '!';

    protected:
        virtual TParsedToken::TValueType TryParseNumber(TStringBuf str) const;

    };

} // namespace NAlice
