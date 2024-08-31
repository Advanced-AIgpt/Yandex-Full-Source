#include "automotive_demo_2019.h"

#include <alice/bass/libs/fetcher/neh.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/library/analytics/common/product_scenarios.h>

namespace NBASS {

namespace {


const TMap<TStringBuf, TString> DEMOMOBILE_COMMANDS =
{
    {TStringBuf("personal_assistant.scenarios.demomobile2019_blink"),        TString("light?action=flash")},
    {TStringBuf("personal_assistant.scenarios.demomobile2019_close_trunk"),  TString("trunk?action=close")},
    {TStringBuf("personal_assistant.scenarios.demomobile2019_close_windows"),TString("windows?action=close")},
    {TStringBuf("personal_assistant.scenarios.demomobile2019_horn"),         TString("beep?action=hello")},
    {TStringBuf("personal_assistant.scenarios.demomobile2019_lock_car"),     TString("lock?action=on")},
    {TStringBuf("personal_assistant.scenarios.demomobile2019_off"),          TString("ignition?action=off")},
    {TStringBuf("personal_assistant.scenarios.demomobile2019_on"),           TString("ignition?action=on")},
    {TStringBuf("personal_assistant.scenarios.demomobile2019_open_trunk"),   TString("trunk?action=open")},
    {TStringBuf("personal_assistant.scenarios.demomobile2019_open_windows"), TString("windows?action=open")},
    {TStringBuf("personal_assistant.scenarios.demomobile2019_unlock_car"),   TString("lock?action=off")},
};

const TMap<TStringBuf, TString> DEMOAURUS_COMMANDS =
{
    {TStringBuf("personal_assistant.scenarios.demoaurus2019_open_windows"),       TString("windows?action=open")},
    {TStringBuf("personal_assistant.scenarios.demoaurus2019_close_windows"),      TString("windows?action=close")},
    {TStringBuf("personal_assistant.scenarios.demoaurus2019_open_left_window"),   TString("window_left?action=open")},
    {TStringBuf("personal_assistant.scenarios.demoaurus2019_open_right_window"),  TString("window_right?action=open")},
    {TStringBuf("personal_assistant.scenarios.demoaurus2019_close_left_window"),  TString("window_left?action=close")},
    {TStringBuf("personal_assistant.scenarios.demoaurus2019_close_right_window"), TString("window_right?action=close")},
    {TStringBuf("personal_assistant.scenarios.demoaurus2019_open_blinds"),        TString("blinds?action=open")},
    {TStringBuf("personal_assistant.scenarios.demoaurus2019_close_blinds"),       TString("blinds?action=close")},
    {TStringBuf("personal_assistant.scenarios.demoaurus2019_temperature_up"),     TString("hvac?action=temperature_up")},
    {TStringBuf("personal_assistant.scenarios.demoaurus2019_temperature_down"),   TString("hvac?action=temperature_down")},
    {TStringBuf("personal_assistant.scenarios.demoaurus2019_partition_up"),       TString("compartment_curtain?action=up")},
    {TStringBuf("personal_assistant.scenarios.demoaurus2019_partition_down"),     TString("compartment_curtain?action=down")},
    {TStringBuf("personal_assistant.scenarios.demoaurus2019_blackout_on"),        TString("blackout?action=on")},
    {TStringBuf("personal_assistant.scenarios.demoaurus2019_blackout_off"),       TString("blackout?action=off")},
    {TStringBuf("personal_assistant.scenarios.demoaurus2019_hide_me"),            TString("hideme")},
    {TStringBuf("personal_assistant.scenarios.demoaurus2019_screen_on"),          TString("screen?action=on")},
    {TStringBuf("personal_assistant.scenarios.demoaurus2019_screen_off"),         TString("screen?action=off")}
};

const TVector<TStringBuf> DEMOMOBILE_CARS = { TStringBuf("ycar1"), TStringBuf("ycar2") };
const TVector<TStringBuf> DEMOAURUS_CARS = {
    TStringBuf("ycaraurus1"), TStringBuf("ycaraurus2"), TStringBuf("ycaraurus2"),
    TStringBuf("ycarauruslimo1"), TStringBuf("ycarauruslimo2"), TStringBuf("ycarauruslimo3")
};

const TStringBuf DEMOMOBILE_EXPERIMENT = "demomobile2019";
const TStringBuf DEMOAURUS_EXPERIMENT = "demoaurus2019";

TString FindYCarId(const TContext& ctx, const TVector<TStringBuf>& experiments) {
    for(const auto& experiment: experiments) {
        if (ctx.HasExpFlag(experiment)) {
            return TString(experiment.begin() + strlen("ycar"), experiment.end());
        }
    }
    return TString();
}

NHttpFetcher::TResponse::TRef MakeRequest(TStringBuf url, const TContext& ctx) {
    NHttpFetcher::TRequestOptions options{};
    options.Timeout = TDuration::MilliSeconds(400);
    options.MaxAttempts = 1;

    NHttpFetcher::TRequestPtr request = NHttpFetcher::Request(url, options);

    if (!request) {
        return {};
    }

    ProxyRequestViaZora(ctx.GetConfig(), request.Get(), ctx.GlobalCtx().SourcesRegistryDelegate());
    request->AddHeader("Cache-Control", "max-age=0");

    return request->Fetch()->Wait();
}

} // namespace

TResultValue TAutomotiveDemo2019Handler::Do(TRequestHandler& r) {
    auto& ctx = r.Ctx();
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::INNER_EVENTS);
    if (!ctx.HasExpFlag(Experiment)) {
        LOG(ERR) << "No automotive demo experiment flag found" << Endl;
        ctx.AddErrorBlock(TError(TError::EType::NOTSUPPORTED, TStringBuf("No automotive demo experiment flag found")));
        return TResultValue();
    }

    TString carId = FindYCarId(ctx, CarIds);
    if (carId.Empty()) {
        LOG(ERR) << "automotive demo 2019: No experiment flag with car id" << Endl;
        ctx.AddErrorBlock(TError(TError::EType::NOTSUPPORTED, TStringBuf("automotive demo 2019: No experiment flag with car id")));
        return TResultValue();
    }

    TStringBuf formName = ctx.FormName();

    auto commandIt = Commands.find(formName);
    if (commandIt == Commands.end()) {
        LOG(ERR) << "automotive demo2019: unknown command " << formName << Endl;
        ctx.AddErrorBlock(TError(TError::EType::INVALIDPARAM, TStringBuf("automotive demo2019: unknown command")));
        return TResultValue();
    }

    TString scheme = "http://";
    if (ctx.HasExpFlag(TStringBuf("ycar_https"))) {
        scheme = "https://";
    }

    TStringBuilder url;
    url << scheme << "130.193.62.68/";
    url << carId;
    url << "/" << commandIt->second;

    auto response = MakeRequest(url, ctx);
    if (!response) {
        LOG(ERR) << "automotive demo2019: car has not responded " << Endl;
        ctx.AddErrorBlock(TError(TError::EType::SYSTEM, TStringBuf("automotive demo 2019: no response from car")));
        return TResultValue();
    }

    if (response && response->IsError()) {
        LOG(ERR) << "automotive demo2019: HTTP error: " << response->GetErrorText() << url << Endl;
        ctx.AddErrorBlock(TError(TError::EType::SYSTEM, TStringBuf("automotive demo2019: HTTP error")));
        return TResultValue();
    } else {
        LOG(INFO) << "automotive demo2019: Command " << commandIt->first << " executed" << Endl;
    }

    return TResultValue();
}

void TAutomotiveDemo2019Handler::Register(THandlersMap* handlers) {
    auto cbDemoMobile2019Handler = []() {
        return MakeHolder<TAutomotiveDemo2019Handler>(
            DEMOMOBILE_EXPERIMENT, DEMOMOBILE_COMMANDS, DEMOMOBILE_CARS);
    };

    for (const auto& pair : DEMOMOBILE_COMMANDS) {
        handlers->emplace(pair.first, cbDemoMobile2019Handler);
    }

    auto cbDemoAurus2019Handler = []() {
        return MakeHolder<TAutomotiveDemo2019Handler>(
            DEMOAURUS_EXPERIMENT, DEMOAURUS_COMMANDS, DEMOAURUS_CARS);
    };

    for (const auto& pair : DEMOAURUS_COMMANDS) {
        handlers->emplace(pair.first, cbDemoAurus2019Handler);
    }
}

}
