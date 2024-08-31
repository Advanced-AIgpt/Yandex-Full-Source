#include "bluetooth_scene.h"

#include <alice/hollywood/library/framework/core/codegen/gen_directives.pb.h>
#include <alice/megamind/protos/common/device_state.pb.h>

namespace NAlice::NHollywoodFw::NBluetooth {

TBluetoothSceneOn::TBluetoothSceneOn(const TScenario* owner)
    : TScene(owner, SCENE_NAME_ON)
{
    RegisterRenderer(&TBluetoothSceneOn::Render);
}

TRetMain TBluetoothSceneOn::Main(
        const TBluetoothSceneArgs&,
        const TRunRequest& runRequest,
        TStorage&,
        const TSource&) const
{
    auto deviceState = runRequest.Client().TryGetMessage<TDeviceState>();
    Y_ENSURE(deviceState, "Can't extract device state");

    TBluetoothOnRenderArgs args;
    args.SetHasConnection(!deviceState->GetBluetooth().GetCurrentConnections().empty());
    LOG_INFO(runRequest.Debug().Logger()) << "bluetooth scenario: winner";
    return TReturnValueRender(&TBluetoothSceneOn::Render, args);
}

TRetResponse TBluetoothSceneOn::Render(
        const TBluetoothOnRenderArgs& args,
        TRender& render)
{
    render.CreateFromNlg("bluetooth", "render_result", args);
    if (!args.GetHasConnection()) {
        render.Directives().AddStartBluetoothDirective();
    }
    return TReturnValueSuccess();
}

TBluetoothSceneOff::TBluetoothSceneOff(const TScenario* owner)
    : TScene(owner, SCENE_NAME_OFF)
{
    RegisterRenderer(&TBluetoothSceneOff::Render);
}

TRetMain TBluetoothSceneOff::Main(
        const TBluetoothSceneArgs&,
        const TRunRequest& runRequest,
        TStorage&,
        const TSource&) const
{
    LOG_INFO(runRequest.Debug().Logger()) << "bluetooth scenario: winner";
    return TReturnValueRender(&TBluetoothSceneOff::Render, {});
}

TRetResponse TBluetoothSceneOff::Render(
        const TBluetoothOffRenderArgs& args,
        TRender& render)
{
    render.CreateFromNlg("bluetooth", "render_result", args);
    render.Directives().AddStopBluetoothDirective();
    return TReturnValueSuccess();
}

TBluetoothSceneUnsupported::TBluetoothSceneUnsupported(const TScenario* owner)
    : TScene(owner, SCENE_NAME_UNSUPPORTED)
{
    RegisterRenderer(&TBluetoothSceneUnsupported::Render);
}

TRetMain TBluetoothSceneUnsupported::Main(
        const TBluetoothSceneArgs&,
        const TRunRequest& runRequest,
        TStorage&,
        const TSource&) const
{
    LOG_INFO(runRequest.Debug().Logger()) << "bluetooth scenario: winner";
    return TReturnValueRender(&TBluetoothSceneUnsupported::Render, {});
}

TRetResponse TBluetoothSceneUnsupported::Render(
        const TBluetoothUnsupportedRenderArgs& args,
        TRender& render)
{
    render.CreateFromNlg("bluetooth", "render_unsupported", args);
    return TReturnValueSuccess();
}

}  // namespace NAlice::NHollywood::NBluetooth
