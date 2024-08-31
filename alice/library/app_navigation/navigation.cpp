#include "navigation.h"

#include <alice/library/url_builder/url_builder.h>

namespace NAlice {

NSc::TValue CreateNavigationBlock(TStringBuf text, TStringBuf tts, TStringBuf app, TStringBuf url, const TClientFeatures& client) {
    NSc::TValue result;
    result["text"] = text;
    result["tts"] = tts;
    result["url"] = app ? NAlice::GenerateApplicationUri(client, app, url) : url;
    return result;
}

NSc::TValue CreateNavigationBlock(TStringBuf text, TStringBuf tts, TStringBuf app, TStringBuf url, const NScenarios::TInterfaces& interfaces, const TClientInfo& clientInfo) {
    NSc::TValue result;
    result["text"] = text;
    result["tts"] = tts;
    result["url"] = app ? NAlice::GenerateApplicationUri(interfaces.GetCanOpenLinkIntent(), clientInfo, app, url) : url;
    return result;
}

NSc::TValue CreateNavBlockImpl(const NSc::TValue& data, const TClientFeatures& client, const TString& navBlockName) {
    NSc::TValue navBlock;

    if (data.IsNull()) {
        return NSc::Null();
    }

    const NSc::TValue& disableExperiment = data["disable_experiment"];
    if (disableExperiment.IsString() && client.HasExpFlag(disableExperiment.GetString())) {
        return NSc::Null();
    }

    const NSc::TValue& experiments = data["experiments"];
    if (!experiments.IsNull()) {
        bool enabled = true;
        if (experiments.IsString()) {
            if (!client.HasExpFlag(experiments.GetString())) {
                enabled = false;
            }
        } else if (experiments.IsArray()) {
            for (const auto& experiment : experiments.GetArray()) {
                if (!client.HasExpFlag(experiment.GetString())) {
                    enabled = false;
                }
            }
        }
        if (!enabled) {
            return NSc::Null();
        }
    }

    const NSc::TValue& nav = data[navBlockName];
    if (nav.IsNull()) {
        return NSc::Null();
    }
    navBlock["text"] = ToString(nav["text"].GetString());
    if (navBlock["text"].GetString().empty()) {
        return NSc::Null();
    }
    navBlock["tts"] = ToString(nav["tts"].GetString());
    if (nav.Has("fallback_tts")) {
        navBlock["fallback_tts"] = nav["fallback_tts"].GetString();
    }
    if (nav.Has("fallback_text")) {
        navBlock["fallback_text"] = nav["fallback_text"].GetString();
    }
    TString app;
    if (client.IsAndroid()) {
        app = nav["app"]["gplay"].GetString();
    } else if (client.IsIOS()) {
        app = nav["app"]["itunes"].GetString();
    }
    TString url;
    const NSc::TValue& urlNode = nav["url"];
    if (urlNode.IsString()) {
        url = nav["url"].GetString();
    } else if (urlNode.IsDict()) {
        const TStringBuf defaultUrl = urlNode["_"].GetString();
        if (client.IsAndroid()) {
            url = urlNode["android"].GetString(defaultUrl);
        } else if (client.IsIOS()) {
            url = urlNode["ios"].GetString(defaultUrl);
        } else if (!client.IsTouch()) {
            url = urlNode["desktop"].GetString(defaultUrl);
        } else {
            url = defaultUrl;
        }
    }
    if (url.empty()) {
        if (app.empty()) {
            return NSc::Null();
        }
        if (client.IsAndroid()) {
            url = CreateGooglePlayAppUrl(app);
        } else if (client.IsIOS()) {
            url = CreateITunesAppUrl(app);
        }
    }
    navBlock["url"] = url;
    navBlock["app"] = app;

    navBlock["voice_name"] = nav["voice_name"];
    navBlock["text_name"] = nav["text_name"];
    navBlock["turboapp"] = data["turboapp"];
    if (nav.Has("intent") && nav["intent"].IsString()) {
        navBlock["intent"] = nav["intent"].GetString();
    }
    navBlock["close_dialog"] = nav["close_dialog"].GetBool(false);
    return navBlock;
}

NSc::TValue CreateNavBlockImpl(const NSc::TValue& data, const NHollywood::TScenarioBaseRequestWrapper& request, const TString& navBlockName) {
    // switch to legacy code, it's safe
    TClientFeatures clientFeatures{request.BaseRequestProto().GetClientInfo(), request.ExpFlags()};
    return CreateNavBlockImpl(data, clientFeatures, navBlockName);
}

NSc::TValue CreateNavBlock(const NSc::TValue& data, const TClientFeatures& client, bool preferApp, const TString& navBlockName) {
    NSc::TValue nav = CreateNavBlockImpl(data, client, navBlockName);
    if (!nav.IsNull() && preferApp && nav.Has("app")) {
        nav["url"] = NAlice::GenerateApplicationUri(client, nav["app"].GetString(), nav["url"].GetString());
    }
    return nav;
}

NSc::TValue CreateNavBlock(const NSc::TValue& data, const NHollywood::TScenarioBaseRequestWrapper& request, bool preferApp, const TString& navBlockName) {
    NSc::TValue nav = CreateNavBlockImpl(data, request, navBlockName);
    if (!nav.IsNull() && preferApp && nav.Has("app")) {
        nav["url"] = NAlice::GenerateApplicationUri(request.Interfaces().GetCanOpenLinkIntent(), request.ClientInfo(), nav["app"].GetString(), nav["url"].GetString());
    }
    return nav;
}

} // namespace NAlice
