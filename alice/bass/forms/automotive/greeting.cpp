#include "greeting.h"

#include <alice/bass/forms/common/personal_data.h>
#include <alice/bass/forms/geodb.h>
#include <alice/bass/forms/traffic.h>
#include <alice/bass/forms/common/personal_data.h>
#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/weather/weather.h>

#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/library/analytics/common/product_scenarios.h>


namespace NBASS {
namespace NAutomotive {

namespace {

constexpr TStringBuf GREETING_FORM_NAME = "personal_assistant.automotive.greeting";

constexpr TStringBuf MUSIC_PROMO_2020 = "auto_music_promo_2020";
constexpr int MAX_MUSIC_PROMO_COUNT = 3;

} // namespace <anonymous>

NHttpFetcher::TResponse::TRef MakeRequest(TContext& ctx, const NHttpFetcher::TRequestPtr& request) {
    Y_ASSERT(request.Get() != nullptr);

    TPersonalDataHelper personalDataHelper(ctx);
    TString tvmUserTicket;
    if (!personalDataHelper.GetTVM2UserTicket(tvmUserTicket)) {
        LOG(ERR) << "Cannot get user ticket" << Endl;
    }
    request->AddHeader("X-Ya-User-Ticket", tvmUserTicket);
    return request->Fetch()->Wait();
}

NSc::TValue GetGreetingFromDrive(TContext& ctx) {
    const auto response = MakeRequest(ctx, ctx.GetSources().CarsharingGreetingPhrase().Request());
    if (response.Get() == nullptr || !response->IsHttpOk()) {
        LOG(ERR) << "Failed requests to drive: " << response->GetErrorText() << Endl;
        return {};
    }
    /* expected response is:
    {
        "settings": [
            {
                "name": "Приветствие Алисы",
                "value": "Привет, Юзер.",
                "type": "string",
                "id": "alice_phrase"
            },
            ...
        ]
    }
    */
    NSc::TValue greetingConfig;
    if (!NSc::TValue::FromJson(greetingConfig, response->Data)) {
        LOG(ERR) << "Cannot parse drive response: " << response->Data << Endl;
        return {};
    }

    for (const auto& setting: greetingConfig["settings"].GetArray()) {
        if (setting["id"] == "alice_phrase") {
            const auto greeting = setting["value"].GetString();
            if (!greeting.empty() && !greeting.EndsWith("."))
                return NSc::TValue(TString(greeting) + ".");
            return greeting;
        }
    }
    return {};
}

TResultValue SetTrafficInfo(TContext& ctx, TRequestedGeo& geo) {
    Y_ASSERT(geo.IsValidId());

    NSc::TValue trafficJson;
    NGeobase::TRegion region = geo.GetRegion();
    NTraffic::GetTrafficInfo(ctx, &trafficJson, region);

    NGeobase::TRegion resolvedGeo = trafficJson.IsNull() ? geo.GetRegion() : region;
    geo.ConvertTo(resolvedGeo.GetId());
    geo.CreateResolvedMeta(ctx, "traffic_resolved_where");

    if (!trafficJson.IsNull()) {
        ctx.CreateSlot("traffic_info", "traffic_info", true, std::move(trafficJson));
    }
    return TResultValue();
}

TResultValue SetWeatherInfo(TContext& ctx, TRequestedGeo& geo) {
    NSc::TValue weatherJson;
    TMaybe<NGeobase::TRegion> respondedGeo;

    // Always request by user coordinates
    const TWeatherFormHandler::TErrorStatus error = TWeatherFormHandler::RequestWeather(
        ctx, geo, &weatherJson, respondedGeo, true /* currentPosition */
    );
    if (error) {
        LOG(ERR) << error->Error.Msg << Endl;
    }
    if (!error && !weatherJson.IsNull()) {
        const auto& l10n = weatherJson["l10n"];
        ctx.CreateSlot("weather_type_current", "string", true, std::move(l10n.Get(weatherJson["fact"]["condition"].GetString())));
        ctx.CreateSlot("weather_temp_current", "num", true, std::move(weatherJson["fact"]["temp"]));
        if (respondedGeo.Defined()) {
            geo.ConvertTo(respondedGeo.GetRef().GetId());
            geo.CreateResolvedMeta(ctx, "weather_resolved_where");
        }
    }
    return TResultValue();
}

// when we can't get counter value, assume that the limit has been reached
bool PromoPhraseLimitReached(TContext& ctx, const TStringBuf promo, int maxCount)
{
    TPersonalDataHelper personalDataHelper(ctx);

    TString puid;
    if (!personalDataHelper.GetUid(puid)) {
        LOG(ERR) << "Couldn't get user id" << Endl;
        return true;
    }

    TString value;
    const TResultValue datasyncErr = personalDataHelper.GetDataSyncKeyValue(
        puid, TPersonalDataHelper::EUserSpecificKey::AutomotivePromoCounters, value
    );

    if (datasyncErr && !(*datasyncErr == TError::EType::NODATASYNCKEYFOUND)) {
        LOG(ERR) << "Datasyc key error: " << datasyncErr << Endl;
    }

    NSc::TValue promoCounters;
    if (!NSc::TValue::FromJson(promoCounters, value)) {
        LOG(ERR) << "Invalid " << ToString(TPersonalDataHelper::EUserSpecificKey::AutomotivePromoCounters)
                 << " json '" << value << "' for uid " << puid << Endl;
    }

    if (promoCounters[promo].GetIntNumberMutable(0) >= maxCount) {
        LOG(INFO) << "limit reached" << Endl;
        return true;
    }

    ++promoCounters[promo].GetIntNumberMutable(0);
    TPersonalDataHelper::TKeyValue keyValue {
        TPersonalDataHelper::EUserSpecificKey::AutomotivePromoCounters, promoCounters.ToJson()
    };

    LOG(INFO) << "Saving datasync data: "
              << TPersonalDataHelper::EUserSpecificKey::AutomotivePromoCounters << " = "
              << promoCounters.ToJson() << Endl;

    if (const auto err = personalDataHelper.SaveDataSyncKeyValues(puid, {keyValue})) {
        LOG(ERR) << "Datasync save data error: " << err << Endl;
    }
    return false;
}

bool MusicPromoNeeded(TContext& ctx)
{
    if (!ctx.HasExpFlag("auto_music_promo") &&
        !ctx.HasExpFlag("auto_music_promo_without_music_start"))
    {
        LOG(DEBUG) << "no music promo experiment" << Endl;
        return false;
    }

    if (!ctx.Meta().DeviceState().HasCarOptions()) {
        LOG(DEBUG) << "no car options" << Endl;
        return false;
    }

    const auto type = ctx.Meta().DeviceState().CarOptions().Type();
    if (type != "carsharing") {
        LOG(DEBUG) << "is not carsharing" << Endl;
        return false;
    }
    return !NAutomotive::PromoPhraseLimitReached(ctx, NAutomotive::MUSIC_PROMO_2020, MAX_MUSIC_PROMO_COUNT);
}

} // namespace NAutomotive

TResultValue TAutomotiveGreetingFormHandler::Do(TRequestHandler& r) {
    auto& ctx = r.Ctx();
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::COMMANDS_OTHER);

    TRequestedGeo geo(ctx, TStringBuf("where"));
    TResultValue err;

    const auto type = ctx.Meta().DeviceState().CarOptions().Type();
    if (type == "carsharing") {
        const auto greetingPhrase = NAutomotive::GetGreetingFromDrive(ctx);
        if (!greetingPhrase.IsNull() && !greetingPhrase.GetString().empty()) {
            ctx.CreateSlot("phrase", "string", true, std::move(greetingPhrase));
        }
    }

    if (geo.HasError()) {
        err = TError(TError::EType::NOGEOFOUND, "no geo found for slot");
    } else {
        err = NAutomotive::SetTrafficInfo(ctx, geo);
        if (err) {
            ctx.AddErrorBlock(*err);
        }
        err = NAutomotive::SetWeatherInfo(ctx, geo);
    }

    if (err) {
        ctx.AddErrorBlock(*err);
    } else if (NAutomotive::MusicPromoNeeded(ctx)) {
        ctx.CreateSlot("promo", "string", true, "music");
    }

    ctx.AddStopListeningBlock();
    return TResultValue();
}

void TAutomotiveGreetingFormHandler::Register(THandlersMap* handlers) {
    auto cbGreetingForm = []() {
        return MakeHolder<TAutomotiveGreetingFormHandler>();
    };
    handlers->emplace(NAutomotive::GREETING_FORM_NAME, cbGreetingForm);
}

} // namespace NBASS
