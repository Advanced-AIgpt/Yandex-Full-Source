#include "simple_intents.h"

#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/route.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <util/generic/hash.h>

namespace NBASS {

namespace {

constexpr TStringBuf NAVI_CHANGE_VOICE = "personal_assistant.navi.change_voice";
constexpr TStringBuf NAVI_CHANGE_VOICE_ELLIPSIS = "personal_assistant.navi.change_voice__ellipsis";

constexpr TStringBuf NAVI_GAS_STATION_WITH_PAYMENT = "personal_assistant.navi.gas_station_with_payment";

constexpr TStringBuf NAVI_HIDE_LAYER = "personal_assistant.navi.hide_layer";
constexpr TStringBuf NAVI_SHOW_LAYER = "personal_assistant.navi.show_layer";

constexpr TStringBuf NAVI_PARKING_ROUTE = "personal_assistant.navi.parking_route";

constexpr TStringBuf NAVI_RESET_ROUTE = "personal_assistant.navi.reset_route";
constexpr TStringBuf NAVI_SHOW_ROUTE = "personal_assistant.navi.show_route_on_map";

class TChangeVoiceIntent : public INavigatorIntent {
public:
    explicit TChangeVoiceIntent(TContext& ctx)
        : INavigatorIntent(ctx, TStringBuf("set_sound_scheme") /* scheme */)
    {}

private:
    TResultValue SetupSchemeAndParams() override {
        TContext::TSlot* selectedVoiceSlot = Context.GetOrCreateSlot(TStringBuf("voice"), TStringBuf("navi_voice"));

        if (IsSlotEmpty(selectedVoiceSlot)) {
            selectedVoiceSlot->Optional = false;
            return TResultValue();
        }

        TStringBuf selectedVoice = selectedVoiceSlot->Value.GetString();
        bool voiceAvailible = false;
        for (const auto& voice : Context.Meta().DeviceState().NavigatorState().AvailibleVoices()) {
            if (selectedVoice == voice.Get()) {
                voiceAvailible = true;
                break;
            }
        }

        if (!voiceAvailible) {
            Context.AddErrorBlock(TError::EType::INVALIDPARAM,
                                  TStringBuilder() << "Unavailible Navigator voice : " << selectedVoice);
        }

        Params.InsertUnescaped(TStringBuf("welcome"), TStringBuf("1"));
        Params.InsertUnescaped(TStringBuf("scheme"), selectedVoice);

        return TResultValue();
    }

    TDirectiveFactory::TDirectiveIndex GetDirectiveIndex() override {
        return GetAnalyticsTagIndex<TNavigatorChangeVoiceDirective>();
    }
};

class TSwitchLayerIntent : public INavigatorIntent {
public:
    TSwitchLayerIntent(TContext& ctx, bool turnOn)
        : INavigatorIntent(ctx)
        , TurnOn(turnOn)
    {}

private:
    TResultValue SetupSchemeAndParams() override {
        TContext::TSlot* layer = Context.GetOrCreateSlot(TStringBuf("layer"), TStringBuf("layer"));
        if (IsSlotEmpty(layer)) {
            layer->Optional = false;
        }

        static const THashMap<TString, TString> roadEvents = {
            {"accidents", "Accident"},
            {"cameras", "SpeedCamera,LaneCamera"},
            {"roadworks", "Reconstruction"},
            {"talks", "Chat"}
        };

        TStringBuf layerName = layer->Value.GetString();
        THashMap<TString, TString>::const_iterator event = roadEvents.find(layerName);
        if (event != roadEvents.cend()) {
            Scheme = "update_flag_setting";
            Params.InsertUnescaped(TStringBuf("name"), TStringBuf("mapRoadEvents"));
            TStringBuf command = (TurnOn ? "on" : "off");
            Params.InsertUnescaped(command, event->second);
        } else if (layerName == "traffic") {
            Scheme = "traffic";
            Params.InsertUnescaped(TStringBuf("traffic_on"), TStringBuf(TurnOn ? "1" : "0"));
        } else if (layerName == "parking") {
            Scheme = "show_ui/map";
            Params.InsertUnescaped(TStringBuf("carparks_enabled"), TStringBuf(TurnOn ? "1" : "0"));
        } else if (layerName == "map_satellite" || layerName == "map_simple") {
            Scheme = "set_setting";
            Params.InsertUnescaped(TStringBuf("name"), TStringBuf("rasterMode"));
            bool mapLayer = (layerName == "map_simple");
            Params.InsertUnescaped(TStringBuf("value"), TStringBuf((mapLayer == TurnOn) ? "Map" : "Sat"));
        }

        return TResultValue();
    }

    TDirectiveFactory::TDirectiveIndex GetDirectiveIndex() override {
        if (Scheme == "update_flag_setting") {
            return GetAnalyticsTagIndex<TNavigatorUpdateSettingDirective>();
        }
        if (Scheme == "traffic") {
            return GetAnalyticsTagIndex<TNavigatorLayerTrafficDirective>();
        }
        if (Scheme == "show_ui/map") {
            return GetAnalyticsTagIndex<TNavigatorShowMapDirective>();
        }
        if (Scheme == "set_setting") {
            return GetAnalyticsTagIndex<TNavigatorSetSettingsDirective>();
        }

        return GetAnalyticsTagIndex<TNavigatorUnknownSchemeDirective>();
    }

private:
    bool TurnOn;
};

class TParkingRouteIntent : public ISchemeOnlyNavigatorIntent {
public:
    explicit TParkingRouteIntent(TContext& ctx)
        : ISchemeOnlyNavigatorIntent(ctx, TStringBuf("carparks_route") /* scheme */)
    {}

private:
    TDirectiveFactory::TDirectiveIndex GetDirectiveIndex() override {
        return GetAnalyticsTagIndex<TNavigatorParkingRouteDirective>();
    }
};

class TShowRouteIntent : public INavigatorIntent {
public:
    explicit TShowRouteIntent(TContext& ctx)
        : INavigatorIntent(ctx, TStringBuf("show_route_overview") /* scheme */)
    {}

private:
    TResultValue SetupSchemeAndParams() override {
        if (!Context.Meta().DeviceState().NavigatorState().HasCurrentRoute()) {
            return TRouteFormHandler::SetAsResponse(Context);
        }

        return TResultValue();
    }

    TDirectiveFactory::TDirectiveIndex GetDirectiveIndex() override {
        return GetAnalyticsTagIndex<TNavigatorShowRouteDirective>();
    }
};

class TResetRouteIntent : public INavigatorIntent {
public:
    explicit TResetRouteIntent(TContext& ctx)
        : INavigatorIntent(ctx, TStringBuf("clear_route") /* scheme */)
    {}

private:
    TResultValue SetupSchemeAndParams() override {
        if (!Context.Meta().DeviceState().NavigatorState().HasCurrentRoute()) {
            Context.AddErrorBlock(TError::EType::NOCURRENTROUTE, TStringBuf("No current route in Navigator state"));
            return TResultValue();
        }

        return TResultValue();
    }

    TDirectiveFactory::TDirectiveIndex GetDirectiveIndex() override {
        return GetAnalyticsTagIndex<TNavigatorResetRouteDirective>();
    }
};

class TSearchGasStationsIntent : public ISchemeOnlyNavigatorIntent {
public:
    explicit TSearchGasStationsIntent(TContext& ctx)
        : ISchemeOnlyNavigatorIntent(ctx, TStringBuf("search_gas_stations") /* scheme */)
    {}

private:
    TDirectiveFactory::TDirectiveIndex GetDirectiveIndex() override {
        return GetAnalyticsTagIndex<TNavigatorSearchGasStationsDirective>();
    }
};

} // namespace

TResultValue TNavigatorSimpleIntentsHandler::Do(TRequestHandler& r) {
    TContext& ctx = r.Ctx();
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::NAVI_COMMANDS);

    const TString& formName = ctx.FormName();

    THolder<INavigatorIntent> navigatorIntent;
    if (formName == NAVI_CHANGE_VOICE || formName == NAVI_CHANGE_VOICE_ELLIPSIS) {
        navigatorIntent = MakeHolder<TChangeVoiceIntent>(ctx);
    } else if (formName == NAVI_SHOW_LAYER || formName == NAVI_HIDE_LAYER) {
        navigatorIntent = MakeHolder<TSwitchLayerIntent>(ctx, formName == NAVI_SHOW_LAYER);
    } else if (formName == NAVI_PARKING_ROUTE) {
        navigatorIntent = MakeHolder<TParkingRouteIntent>(ctx);
    } else if (formName == NAVI_SHOW_ROUTE) {
        navigatorIntent = MakeHolder<TShowRouteIntent>(ctx);
    } else if (formName == NAVI_RESET_ROUTE) {
        navigatorIntent = MakeHolder<TResetRouteIntent>(ctx);
    } else if (formName == NAVI_GAS_STATION_WITH_PAYMENT) {
        navigatorIntent = MakeHolder<TSearchGasStationsIntent>(ctx);
    }

    if (navigatorIntent) {
        return navigatorIntent->Do();
    }

    return TResultValue();
}

void TNavigatorSimpleIntentsHandler::Register(THandlersMap* handlers) {
    auto cbNavigatorSimpleIntentForm = []() {
        return MakeHolder<TNavigatorSimpleIntentsHandler>();
    };

    static const TVector<TStringBuf> supportedFormNames = {
        NAVI_CHANGE_VOICE,
        NAVI_CHANGE_VOICE_ELLIPSIS,
        NAVI_SHOW_LAYER,
        NAVI_HIDE_LAYER,
        NAVI_PARKING_ROUTE,
        NAVI_SHOW_ROUTE,
        NAVI_RESET_ROUTE,
        NAVI_GAS_STATION_WITH_PAYMENT
    };

    for (const auto& formName : supportedFormNames) {
        handlers->emplace(formName, cbNavigatorSimpleIntentForm);
    }
}

}
