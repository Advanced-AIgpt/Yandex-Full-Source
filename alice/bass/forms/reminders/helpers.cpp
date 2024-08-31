#include "helpers.h"

#include <alice/bass/forms/context/context.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/library/scenarios/alarm/date_time.h>
#include <alice/library/scenarios/alarm/helpers.h>
#include <alice/library/scled_animations/scled_animations_builder.h>

#include <util/datetime/parser.h>

namespace NBASS::NReminders {
namespace {

constexpr TStringBuf SLOT_NAME_HOW_LONG = "how_long";

NSc::TValue MakeScledTimeDirectiveByPattern(const TString& pattern, const TString& currTimePattern) {
    NSc::TValue payload;

    {
        NAlice::TScledAnimationBuilder animBuilder;

        animBuilder.AddAnim(pattern, /* bright1= */ 0, /* bright2= */ 255, /* durationMs= */ 820, NAlice::TScledAnimationBuilder::AnimModeFromLeft);
        animBuilder.AddDraw(pattern, /* brightness= */ 255, /* durationMs= */ 2100);
        animBuilder.AddAnim(pattern, /* bright1= */ 255, /* bright2= */ 0, /* durationMs= */ 660, NAlice::TScledAnimationBuilder::AnimModeFromRight);

        animBuilder.SetAnim("     *", /* bright1= */ 0, /* bright2= */ 255, /* from= */ 3460, /* to= */ 3820, NAlice::TScledAnimationBuilder::AnimModeFade);
        animBuilder.SetAnim("     *", /* bright1= */ 255, /* bright2= */ 0, /* from= */ 3820, /* to= */ 4140, NAlice::TScledAnimationBuilder::AnimModeFade);

        animBuilder.SetAnim("     *", /* bright1= */ 0, /* bright2= */ 255, /* from= */ 4120, /* to= */ 4500, NAlice::TScledAnimationBuilder::AnimModeFade);
        animBuilder.SetAnim("     *", /* bright1= */ 255, /* bright2= */ 0, /* from= */ 4500, /* to= */ 4860, NAlice::TScledAnimationBuilder::AnimModeFade);

        animBuilder.SetAnim(currTimePattern, /* bright1= */ 0, /* bright2= */ 255, /* from= */ 4860, /* to= */ 5500, NAlice::TScledAnimationBuilder::AnimModeFade);

        {
            NSc::TValue animation;
            animation["name"]  = "animation_1";
            animation["base64_encoded_value"] = animBuilder.PrepareBinary().c_str();
            animation["compression_type"] = "gzip";

            payload["animations"].SetArray()[0]  =  animation;
        }
    }

    payload["animation_stop_policy"] = "play_once";
    return payload;
}

} // namespace

bool AreTimersAndAlarmsEnabled(const TContext& ctx) {
    const auto& client = ctx.ClientFeatures();
    return client.SupportsTimers() && client.SupportsAlarms();
}

void CreateHowLongSlot(TContext& ctx, const NAlice::NScenarios::NAlarm::TDayTime& time) {
    ctx.CreateSlot(SLOT_NAME_HOW_LONG, SLOT_TYPE_TIME, true /* optional */, time.ToValue());
}

TInstant GetCurrentTimestamp(const TContext& ctx) {
    return TInstant::Seconds(ctx.Meta().Epoch());
}

bool CreateDateTime(TInstant epoch, NDatetime::TCivilSecond now, NDatetime::TTimeZone tzNow, NSc::TValue& to) {
    const NDatetime::TCivilSecond time{NDatetime::Convert(epoch, tzNow)};

    NSc::TValue& dateJson = to["date"] = NAlice::NScenarios::NAlarm::DateToValue(now, time);
    // TODO move this logic to DateToValue() -> TDate::ToValue() via enum flags
    if (now.year() == time.year()) {
        dateJson.Delete("years");
    }

    to["time"] = NAlice::NScenarios::NAlarm::TimeToValue(time);;

    to["datetime"]["year"] = time.year();
    to["datetime"]["month"] = time.month();
    to["datetime"]["day"] = time.day();
    to["datetime"]["hour"] = time.hour();
    to["datetime"]["minute"] = time.minute();
    to["datetime"]["tzinfo"] = tzNow.name();
    to["datetime"]["epoch"] = ToString(epoch.TimeT());

    return true;
}

bool CreateDateTime(TStringBuf epochStr, NDatetime::TCivilSecond now, NDatetime::TTimeZone tzNow, NSc::TValue& to) {
    if (time_t epoch = 0; TryFromString(epochStr, epoch)) {
        return CreateDateTime(TInstant::Seconds(epoch), now, tzNow, to);
    }
    return false;
}

bool ConvertFromISO8601(TStringBuf rd, NDatetime::TCivilSecond now, NDatetime::TTimeZone tzNow, NSc::TValue* to) {
    TIso8601DateTimeParser parser;
    if (!parser.ParsePart(rd.data(), rd.size())) {
        LOG(ERR) << "Unable to parse date from iso8601: " << rd << Endl;
        return false;
    }

    const TInstant epoch{parser.GetResult(TInstant())};
    return CreateDateTime(epoch, now, tzNow, *to);
}

NSc::TValue MakeScledTimeDirective(const NDatetime::TCivilSecond& time, const NDatetime::TCivilSecond& currTime) {
    return MakeScledTimeDirectiveByPattern(
        Sprintf("%02u:%02u ", time.hour(), time.minute()),
        Sprintf("%02u:%02u*", currTime.hour(), currTime.minute())
    );
}

NSc::TValue MakeScledTimeDirective(const TDuration& time,  const NDatetime::TCivilSecond& currTime) {
    return MakeScledTimeDirectiveByPattern(
        Sprintf("%02lu:%02lu ", time.Hours(), time.Minutes() % 60),
        Sprintf("%02u:%02u*", currTime.hour(), currTime.minute())
    );
}

} // namespace NBASS::NReminders
