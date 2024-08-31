#include "ether.h"
#include "directives.h"

#include <alice/bass/libs/client/experimental_flags.h>
#include <alice/bass/forms/automotive/media_control.h>
#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/video/utils.h>
#include <alice/bass/util/error.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/video_common/defs.h>
#include <alice/library/video_common/mordovia_webview_defs.h>

#include <library/cpp/scheme/scheme.h>

namespace NBASS {

    namespace {
        constexpr TStringBuf ETHER_SHOW = "personal_assistant.scenarios.ether_show";
        constexpr TStringBuf DEFAULT_ETHER_URL = "https://yandex.ru/portal/video/quasar/home/";
        constexpr TStringBuf EXPERIMENT_ENABLED = "1";
    }

    TResultValue TEtherHandler::Do(TRequestHandler& r) {
        TContext& context = r.Ctx();
        context.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::PLACEHOLDERS);
        if (context.HasExpFlag(EXP_ETHER)) {
            if (context.FormName() == ETHER_SHOW) {
                NSc::TValue payload;

                TStringBuf url = DEFAULT_ETHER_URL;
                if (const TStringBuf flagValue = context.ExpFlag(EXP_ETHER).GetRef(); flagValue != EXPERIMENT_ENABLED) {
                    url = flagValue;
                }

                TStringBuf splash = NAlice::NVideoCommon::DEFAULT_MORDOVIA_MAIN_SCREEN_SPLASH;
                if (const TStringBuf flagValue = context.ExpFlag(NAlice::NVideoCommon::FLAG_MORDOVIA_MAIN_SCREEN_SPLASH).GetRef()) {
                    splash = flagValue;
                }

                TCgiParameters params;
                NVideo::AddDeviceParamsToVideoUrl(params, context);
                if (!params.empty()) {
                    url = ToString(url) + (url.Contains("?") ? "&" : "?") + params.Print();
                }
                payload["url"] = url;
                payload["splash_div"] = splash;

                if (!context.HasExpFlag(NAlice::NVideoCommon::FLAG_DISABLE_VIDEO_MORDOVIA_SPA)) {
                    payload["scenario"] = NAlice::NVideoCommon::VIDEO_STATION_SPA_MAIN_VIEW_KEY;
                } else {
                    payload["scenario"] = "ether";
                }
                context.AddCommand<TMordoviaShow>(TStringBuf("mordovia_show"), payload);
            }
        }

        return TResultValue();
    }

    void TEtherHandler::Register(THandlersMap* handlers) {
        auto handler = [] () {
            return MakeHolder<TEtherHandler>();
        };
        handlers->emplace(ETHER_SHOW, handler);
    }

}
