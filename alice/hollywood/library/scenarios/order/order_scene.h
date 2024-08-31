#pragma once

#include <alice/hollywood/library/scenarios/order/proto/order.pb.h>

#include <alice/hollywood/library/framework/framework.h>

namespace NAlice::NHollywoodFw::NOrder {

inline constexpr TStringBuf SCENE_NAME_ORDER = "order";
inline constexpr TStringBuf FRAME_ORDER = "alice.order.get_status";

struct TOrderFrame : public TFrame {
    TOrderFrame(const TRequest::TInput& input)
        : TFrame(input, FRAME_ORDER)
        , ProviderName(this, "provider_name")
    {
    }

    TOptionalSlot<TString> ProviderName;

};

class TOrderScene : public TScene<TOrderSceneArgs> {
public:
    TOrderScene(const TScenario* owner);

    TRetSetup MainSetup(const TOrderSceneArgs&, 
                        const TRunRequest&, 
                        const TStorage&) const override;

    TRetMain Main(const TOrderSceneArgs&,
                  const TRunRequest&,
                  TStorage&,
                  const TSource&) const override;

    static TRetResponse Render(const TOrderRenderArgs&, TRender&);
};

}  // namespace NAlice::NHollywood::NOrder
