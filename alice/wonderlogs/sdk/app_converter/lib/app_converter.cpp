#include "app_converter.h"

#include <alice/library/client/client_info.h>

namespace NAlice::NWonderSdk {

EAppType ConvertApp(const TString& app) {
    NAlice::TClientInfoProto proto;
    proto.SetAppId(app);
    NAlice::TClientInfo clientInfo(proto);

    if (clientInfo.IsYaBrowserAlphaMobile()) {
        return EAppType::AT_BROWSER_ALPHA;
    } else if (clientInfo.IsYaBrowserBetaMobile()) {
        return EAppType::AT_BROWSER_BETA;
    } else if (clientInfo.IsYaBrowserProdMobile()) {
        return EAppType::AT_BROWSER_PROD;
    } else if (clientInfo.IsYaStroka()) {
        return EAppType::AT_STROKA;
    } else if (clientInfo.IsYaBrowserTestDesktop()) {
        return EAppType::AT_YABRO_BETA;
    } else if (clientInfo.IsYaBrowserProdDesktop()) {
        return EAppType::AT_YABRO_PROD;
    } else if (clientInfo.IsYaLauncher()) {
        return EAppType::AT_LAUNCHER;
    } else if (clientInfo.IsMiniSpeaker()) {
        return EAppType::AT_SMALL_SMART_SPEAKER;
    } else if (clientInfo.IsNavigator()) {
        return EAppType::AT_NAVIGATOR;
    } else if (clientInfo.IsSearchAppTest()) {
        return EAppType::AT_SEARCH_APP_BETA;
    } else if (clientInfo.IsSearchAppProd()) {
        return EAppType::AT_SEARCH_APP_PROD;
    } else if (clientInfo.IsElariWatch()) {
        return EAppType::AT_ELARIWATCH;
    } else if (clientInfo.IsYaAuto()) {
        return EAppType::AT_AUTO;
    } else if (clientInfo.IsQuasar()) {
        return EAppType::AT_QUASAR;
    }

    return EAppType::AT_OTHER;
}

TString AppToString(EAppType app) {
    switch (app) {
        case AT_UNDEFINED:
            return "null";
        case AT_BROWSER_ALPHA:
            return "browser_alpha";
        case AT_BROWSER_BETA:
            return "browser_beta";
        case AT_BROWSER_PROD:
            return "browser_prod";
        case AT_STROKA:
            return "stroka";
        case AT_YABRO_BETA:
            return "yabro_beta";
        case AT_YABRO_PROD:
            return "yabro_prod";
        case AT_LAUNCHER:
            return "launcher";
        case AT_SMALL_SMART_SPEAKER:
            return "small_smart_speakers";
        case AT_NAVIGATOR:
            return "navigator";
        case AT_SEARCH_APP_BETA:
            return "search_app_beta";
        case AT_SEARCH_APP_PROD:
            return "search_app_prod";
        case AT_ELARIWATCH:
            return "elariwatch";
        case AT_AUTO:
            return "auto";
        case AT_QUASAR:
            return "quasar";
        case AT_OTHER:
            return "other";
    }
}

} // namespace NAlice::NWonderSdk
