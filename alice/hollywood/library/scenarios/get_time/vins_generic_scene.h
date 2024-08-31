#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/get_time/proto/get_time.pb.h>

namespace NAlice::NHollywoodFw::NGetTime {

inline constexpr TStringBuf SCENE_NAME_VINS_GENERIC = "vins_generic";

class TVinsGenericScene : public TScene<TGetTimeVinsGenericSceneArgs> {
public:
    TVinsGenericScene(const TScenario* owner);

    TRetSetup MainSetup(const TGetTimeVinsGenericSceneArgs& args,
                        const TRunRequest& request,
                        const TStorage& storage) const override;

    TRetMain Main(const TGetTimeVinsGenericSceneArgs&,
                  const TRunRequest&,
                  TStorage&,
                  const TSource&) const override;

    static TRetResponse Render(const TGetTimeVinsGenericRenderArgs&, TRender&);
};

}  // namespace NAlice::NHollywood::NGetTime
