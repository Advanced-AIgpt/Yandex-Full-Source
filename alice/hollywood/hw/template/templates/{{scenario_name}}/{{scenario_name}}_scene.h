#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/{{scenario_name}}/proto/{{scenario_name}}.pb.h>

namespace NAlice::NHollywoodFw::N{{ScenarioName}} {

inline constexpr TStringBuf SCENE_NAME_DEFAULT = "default";
inline constexpr TStringBuf FRAME_{{FRAME_NAME}} = "{{frame_name}}";

struct T{{ScenarioName}}Frame : public TFrame {
    T{{ScenarioName}}Frame(const TRequest::TInput& input)
        : TFrame(input, FRAME_{{FRAME_NAME}})
        , Nominal(this, "nominal")
        , Age(this, "age", 42)
    {
    }

    TOptionalSlot<TString> Nominal;
    TSlot<i32> Age;
};

class T{{ScenarioName}}Scene : public TScene<T{{ScenarioName}}SceneArgs> {
public:
    T{{ScenarioName}}Scene(const TScenario* owner);

    TRetMain Main(const T{{ScenarioName}}SceneArgs&,
                  const TRunRequest&,
                  TStorage&,
                  const TSource&) const override;

    static TRetResponse Render(const T{{ScenarioName}}RenderArgs&, TRender&);
};

}  // namespace NAlice::NHollywood::N{{ScenarioName}}
