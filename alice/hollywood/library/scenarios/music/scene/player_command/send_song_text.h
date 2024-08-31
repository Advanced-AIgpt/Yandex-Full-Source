#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/music/proto/music.pb.h>

namespace NAlice::NHollywoodFw::NMusic {

class TMusicScenarioScenePlayerCommandSendSongText : public TScene<TMusicScenarioSceneArgsPlayerCommandSendSongText> {
public:
    TMusicScenarioScenePlayerCommandSendSongText(const TScenario* owner);

    TRetMain Main(
        const TMusicScenarioSceneArgsPlayerCommandSendSongText&,
        const TRunRequest&,
        TStorage&,
        const TSource&) const override;

    TRetResponse Render(
        const TMusicScenarioRenderArgsPlayerCommandSendSongText&,
        TRender&) const;
};

} // namespace NAlice::NHollywoodFw::NMusic
