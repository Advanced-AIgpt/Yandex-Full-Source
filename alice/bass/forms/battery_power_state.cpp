#include "battery_power_state.h"

#include <alice/library/analytics/common/product_scenarios.h>

namespace NBASS {

void TBatteryPowerStateFormHandler::Register(THandlersMap* handlers) {
    handlers->emplace(TStringBuf("personal_assistant.scenarios.battery_power_state"), []() { return MakeHolder<TBatteryPowerStateFormHandler>(); });
}

TResultValue TBatteryPowerStateFormHandler::Do(TRequestHandler& r) {
    TContext& ctx = r.Ctx();
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::BATTERY_POWER_STATE);
    if (ctx.ClientFeatures().SupportsBatteryPowerState()) {
        const auto& state = ctx.Meta().DeviceState();
        if (state.HasBattery()) {
            int percent = state.Battery().Percent();
            if (percent && percent >= 0 && percent <= 100) {
                r.Ctx().CreateSlot(
                    TStringBuf("battery_power_state"),
                    TStringBuf("num"),
                    true, /* optional */
                    percent
                );
                return TResultValue();
            }
        }
    }
    ctx.AddAttention("no_info_about_battery_power_state");
    return TResultValue();
}

}
