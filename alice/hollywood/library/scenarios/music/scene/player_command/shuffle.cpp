#include "shuffle.h"

#include <alice/hollywood/library/scenarios/music/common.h>
#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>
#include <alice/hollywood/library/scenarios/music/scene/common/analytics_info.h>
#include <alice/hollywood/library/scenarios/music/scene/common/render.h>
#include <alice/hollywood/library/scenarios/music/scene/common/structs.h>

namespace NAlice::NHollywoodFw::NMusic {

TMusicScenarioScenePlayerCommandShuffle::TMusicScenarioScenePlayerCommandShuffle(const TScenario* owner)
    : TScene{owner, "player_command_shuffle"}
{
}

TRetMain TMusicScenarioScenePlayerCommandShuffle::Main(const TMusicScenarioSceneArgsPlayerCommandShuffle&,
                                                       const TRunRequest& request,
                                                       TStorage& storage,
                                                       const TSource& source) const
{
    static constexpr TStringBuf nlgTemplate = NHollywood::NMusic::TEMPLATE_PLAYER_SHUFFLE;

    // TODO(sparkle): some prikols before all commands
    // TODO(sparkle): IsPlayerCommandApplicable
    TScenarioRequestData requestData{.Request = request, .Storage = storage, .Source = &source};
    TScenarioStateData state{requestData};

    TCommonRenderData renderData;
    auto& nlgData = *renderData.RenderArgs.MutableNlgData();
    (*nlgData.MutableContext()->MutableAttentions())["foo"] = "bar"; // TODO(sparkle): solve radically
    nlgData.SetTemplate(nlgTemplate.data(), nlgTemplate.size());
    nlgData.SetPhrase("render_result");

    // TODO(sparkle): refactor to function
    if (const auto& psn = state.ScenarioState.GetProductScenarioName(); !psn.Empty()) {
        request.AI().OverrideProductScenarioName(psn);
    } else {
        request.AI().OverrideProductScenarioName("player_commands");
    }
    request.AI().OverrideIntent("personal_assistant.scenarios.player_shuffle");

    request.AI().AddAction(CreateAction("player_shuffle", "player shuffle",
                                        "Воспроизведение происходит в случайном порядке"));

    renderData.FillRunFeatures(requestData);

    if (state.MusicQueue.IsGenerative()) {
        nlgData.MutableContext()->SetIsGenerative(true);
    } else if (state.MusicQueue.IsFmRadio()) {
        nlgData.MutableContext()->SetIsFmRadio(true);
        nlgData.MutableContext()->SetFmRadioName(state.MusicQueue.CurrentItem().GetTitle());
    } else if (state.MusicQueue.IsRadio()) {
        // TODO(sparkle): respect `nlg_disabled` slot
        nlgData.MutableContext()->SetIsRadio(true);
    } else {
        state.MusicQueue.ShufflePlayback(request.System().Random());

        LOG_INFO(request.Debug().Logger()) << "Shuffle current content, "
                         << NHollywood::NMusic::TContentId::EContentType_Name(state.MusicQueue.ContentId().GetType())
                         << ":" << state.MusicQueue.ContentId().GetId();
        // TODO(sparkle): request tracks in Continue stage
    }

    return TReturnValueRender(&CommonRender, renderData.RenderArgs, std::move(renderData.RunFeatures));
}

} // namespace NAlice::NHollywoodFw::NMusic
