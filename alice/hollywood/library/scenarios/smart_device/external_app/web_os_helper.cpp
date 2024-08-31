#include "web_os_helper.h"

#include <alice/hollywood/library/environment_state/endpoint.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/protos/endpoint/capability.pb.h>
#include <alice/protos/endpoint/endpoint.pb.h>
#include <alice/library/analytics/common/product_scenarios.h>

namespace NAlice::NHollywood {

namespace {

struct TWebOSApp {
    TString AppId;
    TVector<TString> AvailableApps;
    TString HumanReadableName;
    TString ParamsJson;
};

const THashMap<TString, TWebOSApp> TV_APP_TO_WEB_OS_APP_CONVERTER = {
    {"ru.kinopoisk.yandex.tv", {"tv.kinopoisk.ru",
                               {"tv.kinopoisk.ru", "tv.kinopoisk-hd.ru", "com.685631.3411"},
                               "Kinopoisk",
                               "{\"pageId\":\"library\"}"}},

    {"com.yandex.tv.ytplayer", {"youtube.leanback.v4",
                               {"youtube.leanback.v4"},
                               "Youtube",
                               ""}},
};


TMaybe<TWebOSCapability> GetWebOSCapability(const TScenarioRunRequestWrapper& request) {
    const TEnvironmentState* environmentState = GetEnvironmentStateProto(request);
    if (!environmentState) {
        return Nothing();
    }

    const TEndpoint* webOSEndpoint = FindEndpoint(*environmentState, TEndpoint_EEndpointType_WebOsTvEndpointType);
    if (webOSEndpoint) {
        for (const auto& capability : webOSEndpoint->GetCapabilities()) {
            TWebOSCapability webOSCapability;
            if (capability.UnpackTo(&webOSCapability)) {
                return webOSCapability;
            }
        };
    }

    return Nothing();
}

} // namespace

bool CanLaunchWebOSAppDirective(const TScenarioRunRequestWrapper& request) {
    const auto webOSCapability = GetWebOSCapability(request);
    if (webOSCapability.Defined()) {
        return AnyOf(webOSCapability->GetMeta().GetSupportedDirectives(), [](const auto& directive) {
            return directive == TCapability_EDirectiveType_WebOSLaunchAppDirectiveType;
        });
    }

    return false;
}

EOpenExternalAppResult MakeWebOSLaunchAppDirective(
    const TScenarioRunRequestWrapper& request, TRTLogger& logger,
    const TString& recognizedPackageName, TMaybe<NScenarios::TDirective>& maybeDirective)
{
    Y_UNUSED(request);
    if (!TV_APP_TO_WEB_OS_APP_CONVERTER.contains(recognizedPackageName)) {
        LOG_INFO(logger) << "Application not recognized";
        return EOpenExternalAppResult::UnableOpenUnlistedApp;
    }

    const auto webOSCapability = GetWebOSCapability(request);

    bool appIsInstalled = webOSCapability.Defined() && AnyOf(webOSCapability->GetParameters().GetAvailableApps(), [&recognizedPackageName](const auto& installedApp) {
        return AnyOf(TV_APP_TO_WEB_OS_APP_CONVERTER.at(recognizedPackageName).AvailableApps, [&installedApp](const auto& app) {
            return app == installedApp.GetAppId();
        });
    });
    if (!appIsInstalled) {
        LOG_INFO(logger) << "Application is not installed";
        return EOpenExternalAppResult::UnableOpenRequestedApp;
    }

    NScenarios::TDirective directive;
    auto& webOSLaunchAppDirective = *directive.MutableWebOSLaunchAppDirective();
    webOSLaunchAppDirective.SetName("web_os_launch_app");
    webOSLaunchAppDirective.SetAppId(TV_APP_TO_WEB_OS_APP_CONVERTER.at(recognizedPackageName).AppId);
    webOSLaunchAppDirective.SetParamsJson(TV_APP_TO_WEB_OS_APP_CONVERTER.at(recognizedPackageName).ParamsJson);
    maybeDirective = directive;
    return EOpenExternalAppResult::OpenRequestedApp;
}

} // namespace NAlice::NHollywood
