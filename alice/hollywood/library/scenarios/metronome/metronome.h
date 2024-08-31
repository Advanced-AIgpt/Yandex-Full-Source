#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/metronome/nlg/register.h>

#include <alice/hollywood/library/scenarios/metronome/proto/metronome_render_state.pb.h>
#include <alice/hollywood/library/scenarios/metronome/proto/metronome_scenario_state.pb.h>

namespace NAlice::NHollywoodFw::NMetronome {

inline constexpr TStringBuf SCENE_NAME_START = "start_scene";
inline constexpr TStringBuf SCENE_NAME_UPDATE = "update_scene";

class TMetronomeStartScene : public TScene<TMetronomeScenarioStartArguments> {
public:
    TMetronomeStartScene(const TScenario* owner);
    TRetMain Main(const TMetronomeScenarioStartArguments&,
                  const TRunRequest&,
                  TStorage&,
                  const TSource&) const override;
};

class TMetronomeUpdateScene : public TScene<TMetronomeScenarioUpdateArguments> {
public:
    TMetronomeUpdateScene(const TScenario* owner);
    TRetMain Main(const TMetronomeScenarioUpdateArguments&,
                  const TRunRequest&,
                  TStorage&,
                  const TSource&) const override;
};


class TMetronomeScenario : public TScenario {
public:
    TMetronomeScenario();
    TRetScene Dispatch(const TRunRequest&,
                       const TStorage&,
                       const TSource&) const;

    static TRetResponse RenderStartScene(const TMetronomeRenderStartArguments& args, TRender& render);
    static TRetResponse RenderUpdateScene(const TMetronomeRenderUpdateArguments& args, TRender& render);
    static TRetResponse RenderIrrelevantScene(const TMetronomeRenderIrrelevantArguments& args, TRender& render);
};

}  // namespace NAlice::NHollywoodFw::NMetronome
