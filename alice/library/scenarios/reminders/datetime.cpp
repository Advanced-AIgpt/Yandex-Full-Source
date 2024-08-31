#include "datetime.h"

namespace NAlice::NRemindersApi {
namespace {
} // ns

TDateBounds::TDateBounds(NDatetime::TCivilSecond from, NDatetime::TCivilSecond till, const NDatetime::TTimeZone& timeZone)
    : From_{from}
    , Till_{till}
    , TimeZone_{timeZone}
{
}

TDateBounds& TDateBounds::AdjustRightBound() {
    if (From_ > Till_) {
        Till_ = NDatetime::AddDays(Till_, 1);
    }

    return *this;
}

bool TDateBounds::IsIn(NDatetime::TCivilSecond shootAt) const {
    return shootAt >= From_ && shootAt <= Till_;
}

bool TDateBounds::IsIn(TInstant shootAt) const {
    return IsIn(NDatetime::Convert(shootAt, TimeZone_));
}

TString TDateBounds::FormatFrom(TStringBuf format) const {
    return NDatetime::Format(format, From_, TimeZone_);
}

TString TDateBounds::FormatTill(TStringBuf format) const {
    return NDatetime::Format(format, Till_, TimeZone_);
}

} // namespace NAlice::NRemindersApi
