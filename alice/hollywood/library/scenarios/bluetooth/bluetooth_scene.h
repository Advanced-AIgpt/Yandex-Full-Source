#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/bluetooth/proto/bluetooth.pb.h>

namespace NAlice::NHollywoodFw::NBluetooth {

inline constexpr TStringBuf SCENE_NAME_ON = "scene_on";
inline constexpr TStringBuf SCENE_NAME_OFF = "scene_off";
inline constexpr TStringBuf SCENE_NAME_UNSUPPORTED = "scene_unsupported";
inline constexpr TStringBuf FRAME_BLUETOOTH_ON = "personal_assistant.scenarios.bluetooth_on";
inline constexpr TStringBuf FRAME_BLUETOOTH_OFF = "personal_assistant.scenarios.bluetooth_off";

struct TBluetoothOnFrame : public TFrame {
    TBluetoothOnFrame(const TRequest::TInput& input)
        : TFrame(input, FRAME_BLUETOOTH_ON)
    {
    }
};

struct TBluetoothOffFrame : public TFrame {
    TBluetoothOffFrame(const TRequest::TInput& input)
        : TFrame(input, FRAME_BLUETOOTH_OFF)
    {
    }
};

class TBluetoothSceneOn : public TScene<TBluetoothSceneArgs> {
public:
    TBluetoothSceneOn(const TScenario* owner);

    TRetMain Main(const TBluetoothSceneArgs&,
                  const TRunRequest&,
                  TStorage&,
                  const TSource&) const override;

    static TRetResponse Render(const TBluetoothOnRenderArgs&, TRender&);
};

class TBluetoothSceneOff : public TScene<TBluetoothSceneArgs> {
public:
    TBluetoothSceneOff(const TScenario* owner);

    TRetMain Main(const TBluetoothSceneArgs&,
                  const TRunRequest&,
                  TStorage&,
                  const TSource&) const override;

    static TRetResponse Render(const TBluetoothOffRenderArgs&, TRender&);
};

class TBluetoothSceneUnsupported : public TScene<TBluetoothSceneArgs> {
public:
    TBluetoothSceneUnsupported(const TScenario* owner);

    TRetMain Main(const TBluetoothSceneArgs&,
                  const TRunRequest&,
                  TStorage&,
                  const TSource&) const override;

    static TRetResponse Render(const TBluetoothUnsupportedRenderArgs&, TRender&);
};

}  // namespace NAlice::NHollywood::NBluetooth
