#pragma once

#include <library/cpp/timezone_conversion/civil.h>

#include <util/datetime/base.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NAlice::NRemindersApi {

class TDateBounds {
public:
    TDateBounds(NDatetime::TCivilSecond from, NDatetime::TCivilSecond till, const NDatetime::TTimeZone& timeZone);

    /* Check if `till` time is less than `from`, then adds one day to `till`.
     */
    TDateBounds& AdjustRightBound();

    bool IsIn(TInstant shootAt) const;
    bool IsIn(NDatetime::TCivilSecond shootAt) const;

    TString FormatFrom(TStringBuf format) const;
    TString FormatTill(TStringBuf format) const;

private:
    NDatetime::TCivilSecond From_;
    NDatetime::TCivilSecond Till_;
    NDatetime::TTimeZone TimeZone_;
};

} // namespace NAlice::NRemindersApi
