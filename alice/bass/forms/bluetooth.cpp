#include "bluetooth.h"
#include "directives.h"

#include <alice/bass/forms/automotive/media_control.h>
#include <alice/bass/forms/context/context.h>
#include <alice/bass/util/error.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <library/cpp/scheme/scheme.h>

namespace NBASS {

namespace {

constexpr TStringBuf BLUETOOTH_ON = "personal_assistant.scenarios.bluetooth_on";
constexpr TStringBuf BLUETOOTH_OFF = "personal_assistant.scenarios.bluetooth_off";

} // namespace anonymous

TResultValue TBluetoothHandler::Do(TRequestHandler& r) {
    TContext& context = r.Ctx();
    context.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::BLUETOOTH);
    if (context.MetaClientInfo().IsYaAuto() && context.FormName() == BLUETOOTH_ON) {
        return NAutomotive::HandleMediaControlSource(context, TStringBuf("a2dp"));
    }

    if (context.ClientFeatures().SupportsNoBluetooth()) {
        NSc::TValue unsupportedOperationError;
        unsupportedOperationError["code"].SetString("unsupported_operation");
        context.AddErrorBlock(TError(TError::EType::NOTSUPPORTED, TStringBuf("bluetooth_error")),
                              std::move(unsupportedOperationError));

        return TResultValue();
    }

    if (context.FormName() == BLUETOOTH_ON) {
        if (!context.Meta().DeviceState().Bluetooth().HasCurrentConnections()
            || context.Meta().DeviceState().Bluetooth().CurrentConnections().Empty())
        {
            context.AddCommand<TBluetoothStartDirective>(TStringBuf("start_bluetooth"), NSc::Null());
        } else {
            context.AddAttention("too_many_connections", NSc::Null());
        }
    } else if (context.FormName() == BLUETOOTH_OFF) {
        context.AddCommand<TBluetoothStopDirective>(TStringBuf("stop_bluetooth"), NSc::Null());
    }

    return TResultValue();
}

void TBluetoothHandler::Register(THandlersMap* handlers) {
    auto handler = [] () {
        return MakeHolder<TBluetoothHandler>();
    };
    handlers->emplace(BLUETOOTH_ON, handler);
    handlers->emplace(BLUETOOTH_OFF, handler);
}

} // NBASS
