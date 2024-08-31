#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/blueprints/proto/blueprints.pb.h>

namespace NAlice::NHollywoodFw::NBlueprints {

inline constexpr TStringBuf SCENE_NAME_DEFAULT = "default";

class TBlueprintsScene : public TScene<TBlueprintsArgs> {
public:
    TBlueprintsScene(const TScenario* owner);

    TRetMain Main(const TBlueprintsArgs&,
                  const TRunRequest&,
                  TStorage&,
                  const TSource&) const override;

    static TRetResponse Render(const TBlueprintsArgs&, TRender&);
};

}  // namespace NAlice::NHollywood::NBlueprints
