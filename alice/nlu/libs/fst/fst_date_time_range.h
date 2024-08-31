#pragma once

#include "fst_date_time.h"

#pragma once

namespace NAlice {

    class TFstDateTimeRange : public TFstDateTime {
    public:
        using TFstDateTime::TFstDateTime;

    protected:
        TParsedToken::TValueType ToDateTime(const TVector<TStringBuf>& values) const override;
        const RE2& TapePattern() const override;
    };

} // namespace NAlice

