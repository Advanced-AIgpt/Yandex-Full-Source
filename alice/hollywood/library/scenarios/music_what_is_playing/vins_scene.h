#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/music_what_is_playing/proto/music_what_is_playing.pb.h>

namespace NAlice::NHollywoodFw::NMusicWhatIsPlaying {

class TMusicWhatIsPlayingVinsScene : public TScene<TMusicWhatIsPlayingVinsSceneArgs> {
public:
    TMusicWhatIsPlayingVinsScene(const TScenario* owner);

    TRetSetup MainSetup(const TMusicWhatIsPlayingVinsSceneArgs& args,
                        const TRunRequest& request,
                        const TStorage& storage) const override;

    TRetMain Main(const TMusicWhatIsPlayingVinsSceneArgs& args,
                  const TRunRequest& request,
                  TStorage& storage,
                  const TSource& source) const override;

    TRetSetup ApplySetup(const TMusicWhatIsPlayingVinsSceneArgs& args,
                         const TApplyRequest& request,
                         const TStorage& storage) const override;

    TRetContinue Apply(const TMusicWhatIsPlayingVinsSceneArgs& args,
                       const TApplyRequest& request,
                       TStorage& storage,
                       const TSource& source) const override;

    static TRetResponse RenderRun(const TMusicWhatIsPlayingVinsRunRenderArgs& args,
                                  TRender& render);

    static TRetResponse RenderApply(const TMusicWhatIsPlayingVinsApplyRenderArgs& args,
                                    TRender& render);
public:
    static constexpr TStringBuf SceneName = "vins";
};

}  // namespace NAlice::NHollywoodFw::NMusicWhatIsPlaying
