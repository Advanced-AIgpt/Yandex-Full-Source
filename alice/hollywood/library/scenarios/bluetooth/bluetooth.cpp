#include "bluetooth.h"
#include "bluetooth_scene.h"

#include <alice/hollywood/library/scenarios/bluetooth/nlg/register.h>

#include <alice/library/analytics/common/product_scenarios.h>

namespace NAlice::NHollywoodFw::NBluetooth {

HW_REGISTER(TBluetoothScenario);

TBluetoothScenario::TBluetoothScenario()
    : TScenario(NProductScenarios::BLUETOOTH)
{
    Register(&TBluetoothScenario::Dispatch);
    RegisterScene<TBluetoothSceneOn>([this]() {
        RegisterSceneFn(&TBluetoothSceneOn::Main);
    });
    RegisterScene<TBluetoothSceneOff>([this]() {
        RegisterSceneFn(&TBluetoothSceneOff::Main);
    });
    RegisterScene<TBluetoothSceneUnsupported>([this]() {
        RegisterSceneFn(&TBluetoothSceneUnsupported::Main);
    });

    RegisterRenderer(&TBluetoothScenario::RenderIrrelevant);

    // Additional functions
    SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NBluetooth::NNlg::RegisterAll);
}

TRetScene TBluetoothScenario::Dispatch(
        const TRunRequest& runRequest,
        const TStorage&,
        const TSource&) const
{
    const TBluetoothOnFrame frameOn(runRequest.Input());
    const TBluetoothOffFrame frameOff(runRequest.Input());

    bool isSupported = runRequest.Client().GetInterfaces().GetHasBluetooth();
    if (!isSupported && (frameOn.Defined() || frameOff.Defined())) {
        auto name = frameOn.Defined() ? frameOn.GetName() : frameOff.GetName();
        return TReturnValueScene<TBluetoothSceneUnsupported>(TBluetoothSceneArgs{}, name);
    }

    if (frameOn.Defined()) {
        return TReturnValueScene<TBluetoothSceneOn>(TBluetoothSceneArgs{}, frameOn.GetName());
    }

    if (frameOff.Defined()) {
        return TReturnValueScene<TBluetoothSceneOff>(TBluetoothSceneArgs{}, frameOff.GetName());
    }

    LOG_ERR(runRequest.Debug().Logger()) << "Semantic frames not found";
    return TReturnValueRenderIrrelevant(&TBluetoothScenario::RenderIrrelevant, {});
}

TRetResponse TBluetoothScenario::RenderIrrelevant(
        const TBluetoothRenderIrrelevant&,
        TRender& render)
{
    render.CreateFromNlg("bluetooth", "render_irrelevant", NJson::TJsonValue{});
    return TReturnValueSuccess();
}

}  // namespace NAlice::NsBluetoothHollywood::NBluetooth
