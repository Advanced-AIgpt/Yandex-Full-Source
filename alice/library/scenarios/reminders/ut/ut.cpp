#include "ut.h"

#include <library/cpp/timezone_conversion/civil.h>

namespace NAlice::NRemindersApi::NTesting {

TReminderProto TRemindersApiFixture::CreateValidReminderProto(const TString& id, i64 diffSeconds) const {
    TReminderProto proto;
    proto.SetId(id);
    proto.SetText("remind-me-this");
    proto.SetShootAt(Now.Seconds() + diffSeconds);
    proto.SetTimeZone("UTC");
    return proto;
}

TMaybe<TDateBounds> TRemindersApiFixture::CreateDateBounds(const TParametrizeAction::TFilter* bounds) const {
    if (!bounds) {
        return Nothing();
    }

    const auto tz = NDatetime::GetTimeZone("Europe/Kiev");

    auto from = NDatetime::Convert(TInstant::Seconds(Now.Seconds() + bounds->From), tz);
    auto till = NDatetime::Convert(TInstant::Seconds(Now.Seconds() + bounds->Till), tz);

    TDateBounds db{from, till, tz};
    if (bounds->Adjust) {
        db.AdjustRightBound();
    }
    return std::move(db);
}

} // NAlice::NRemindersApi::NTesting
