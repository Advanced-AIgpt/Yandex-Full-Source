#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/bluetooth/proto/bluetooth.pb.h>

namespace NAlice::NHollywoodFw::NBluetooth {

class TBluetoothScenario : public TScenario {
public:
    TBluetoothScenario();

    TRetScene Dispatch(const TRunRequest&,
                       const TStorage&,
                       const TSource&) const;

    static TRetResponse RenderIrrelevant(const TBluetoothRenderIrrelevant&, TRender&);
};

}  // namespace NAlice::NHollywood::NBluetooth