#include "gettime.h"

#include <alice/bass/libs/config/config.h>
#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/geodb.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/versioning/versioning.h>

#include <library/cpp/timezone_conversion/convert.h>

#include <util/string/join.h>

namespace NBASS {

namespace {

constexpr TStringBuf LED_SCREEN_DIRECTIVE = "draw_led_screen";
constexpr TStringBuf FORCE_DISPLAY_CARDS_DIRECTIVE = "force_display_cards";
constexpr TStringBuf GIF_URI_PREFIX = "https://static-alice.s3.yandex.net/led-production/";
constexpr TStringBuf GIF_PATH_SEP = "/";
constexpr TStringBuf GET_TIME_GIF_VERSION = "1";
constexpr TStringBuf GET_TIME_GIF_DEFAULT_SUBVERSION = "2";

class TTimeRequestImpl {
public:
    TTimeRequestImpl(TRequestHandler& r);
    TResultValue Do();

private:
    TContext& Ctx;
    TRequestedGeo Geo;

    bool ProcessInputSlots();
    void ConvertTimeToOutputDate(const NDatetime::TSimpleTM& civilTime, NSc::TValue* res) const;
};

TTimeRequestImpl::TTimeRequestImpl(TRequestHandler& r)
    : Ctx(r.Ctx())
    , Geo(Ctx.GlobalCtx())
{
}

bool TTimeRequestImpl::ProcessInputSlots() {
    Geo = TRequestedGeo(Ctx, TStringBuf("where"));
    if (Geo.HasError()) {
        NSc::TValue er;
        er["code"] = Geo.GetError()->Type == TError::EType::NOUSERGEO ? TStringBuf("bad_user_geo") : TStringBuf("bad_geo");
        Ctx.AddErrorBlock(*Geo.GetError(), std::move(er));
        return false;
    }
    // Check if Type is smaller than TOWN
    if (Geo.GetGeoType() > NGeobase::ERegionType::VILLAGE) {
        Geo.ConvertTo(Geo.GetParentCityId());
    }

    return true;
}

void TTimeRequestImpl::ConvertTimeToOutputDate(const NDatetime::TSimpleTM& civilTime, NSc::TValue* res) const {
/*
    TSimpleTM fields:
        i32 GMTOff = 0; // -43200 - 50400 seconds
        ui16 Year = 0;  // from 1900
        ui16 YDay = 0;  // 0-365
        ui8 Mon = 0;    // 0-11
        ui8 MDay = 0;   // 1-31
        ui8 WDay = 0;   // 0-6
        ui8 Hour = 0;   // 0-23
        ui8 Min = 0;    // 0-59
        ui8 Sec = 0;    // 0-60 - doesn't care for leap seconds. Most of the time it's ok.
        i8 IsDst = 0;   // -1/0/1
*/
    res->SetDict();
    (*res)["year"] = civilTime.RealYear();
    (*res)["month"] = civilTime.RealMonth();
    (*res)["day"] = civilTime.MDay;
    (*res)["hour"] = civilTime.Hour;
    (*res)["min"] = civilTime.Min;
    (*res)["sec"] = civilTime.Sec;
}

TString MakeWeatherGifUris(const TContext& ctx, const NSc::TValue& timeStruct) {
    const auto versionFlagFull = TString::Join(NAlice::NExperiments::EXP_GIF_VERSION, "get_time", ":");
    const auto subversion = ctx.GetValueFromExpPrefix(
        versionFlagFull
    ).GetOrElse(GET_TIME_GIF_DEFAULT_SUBVERSION);
    const TString gifName = JoinSeq(
        "-",
        {
            TString("clock"),
            ToString(timeStruct["hour"].GetIntNumber() / 10),
            ToString(timeStruct["hour"].GetIntNumber() % 10),
            ToString(timeStruct["min"].GetIntNumber() / 10),
            ToString(timeStruct["min"].GetIntNumber() % 10)
        }
    );
    return NAlice::FormatVersion(
                TString::Join(GIF_URI_PREFIX, "get_time/clocks"),
                TString::Join(gifName, ".gif"),
                GET_TIME_GIF_VERSION,
                subversion,
                /* sep = */ GIF_PATH_SEP
            );
}

void AddLedDirective(TContext& ctx, const TString& imageUri) {
    ctx.AddCommand<TForceDisplayCardsDirective>(FORCE_DISPLAY_CARDS_DIRECTIVE, {});

    NSc::TValue data;
    data["listening_is_possible"].SetBool(true);
    NSc::TValue& item = data["animation_sequence"].Push();
    item["frontal_led_image"] = imageUri;

    ctx.AddCommand<TDrawLedScreenDirective>(LED_SCREEN_DIRECTIVE, std::move(data));
}

TResultValue TTimeRequestImpl::Do() {
    if (!ProcessInputSlots()) {
        Ctx.AddSearchSuggest();
        Ctx.AddOnboardingSuggest();
        return TResultValue();
    }

    TString timezone = Geo.GetTimeZone();
    if (timezone.empty()) {
        Ctx.AddErrorBlock(
            TError(
                TError::EType::NOTIMEZONE,
                TStringBuf("unknown_timezone")
            )
        );
    } else {
        // Always use client timestamp
        TInstant timeNow = TInstant::Seconds(Ctx.Meta().Epoch());
        NDatetime::TSimpleTM civilTime = NDatetime::ToCivilTime(timeNow, NDatetime::GetTimeZone(timezone));

        NSc::TValue timeStruct;
        ConvertTimeToOutputDate(civilTime, &timeStruct);
        timeStruct["timezone"] = timezone;

        Ctx.CreateSlot(TStringBuf("time_result"), TStringBuf("time_result"), true, timeStruct);

        if (Ctx.ClientFeatures().SupportsLedDisplay()) {
            AddLedDirective(Ctx, MakeWeatherGifUris(Ctx, timeStruct));
        }
    }

    NSc::TValue timeLocation;
    Geo.AddAllCaseFormsWithFallbackLanguage(Ctx, &timeLocation, /* wantObsolete = */ true);
    if (!timeLocation["city_prepcase"].StringEmpty()) {
        Ctx.CreateSlot(TStringBuf("time_location"), TStringBuf("geo"), true, timeLocation);
    }

    Ctx.AddSearchSuggest();
    Ctx.AddOnboardingSuggest();
    if (Ctx.MetaClientInfo().IsSmartSpeaker()) {
        Ctx.AddStopListeningBlock();
    }
    return TResultValue();
}

} // end anon namespace

TResultValue TTimeFormHandler::Do(TRequestHandler& r) {
    r.Ctx().GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::GET_TIME);
    TTimeRequestImpl impl(r);
    return impl.Do();
}

void TTimeFormHandler::Register(THandlersMap* handlers) {
    auto cbTimeForm = []() {
        return MakeHolder<TTimeFormHandler>();
    };
    handlers->emplace(TStringBuf("personal_assistant.scenarios.get_time"), cbTimeForm);
    handlers->emplace(TStringBuf("personal_assistant.scenarios.get_time__ellipsis"), cbTimeForm);
}

}
