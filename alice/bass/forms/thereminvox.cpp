#include "thereminvox.h"
#include "directives.h"

#include <alice/bass/libs/client/experimental_flags.h>
#include <alice/bass/forms/automotive/media_control.h>
#include <alice/bass/forms/context/context.h>
#include <alice/bass/util/error.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <library/cpp/scheme/scheme.h>

namespace NBASS {

    namespace {

        constexpr TStringBuf THEREMINVOX_ON = "personal_assistant.scenarios.thereminvox_on";
        constexpr TStringBuf THEREMINVOX_OFF = "personal_assistant.scenarios.thereminvox_off";
    } // namespace anonymous

    TResultValue TThereminvoxHandler::Do(TRequestHandler& r) {
        TContext& context = r.Ctx();
        context.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::THEREMIN);
        if (r.Ctx().HasExpFlag(EXPERIMENTAL_FLAG_THEREMINVOX)) {
            if (r.Ctx().FormName() == THEREMINVOX_ON) {
                context.AddCommand<TThereminvoxStartDirective>(TStringBuf("start_thereminvox"), NSc::Null());
            } else if (r.Ctx().FormName() == THEREMINVOX_OFF) {
                context.AddCommand<TThereminvoxStopDirective>(TStringBuf("stop_thereminvox"), NSc::Null());
            }
        }

        return TResultValue();
    }

    void TThereminvoxHandler::Register(THandlersMap* handlers) {
        auto handler = [] () {
            return MakeHolder<TThereminvoxHandler>();
        };
        handlers->emplace(THEREMINVOX_ON, handler);
        handlers->emplace(THEREMINVOX_OFF, handler);
    }

}
