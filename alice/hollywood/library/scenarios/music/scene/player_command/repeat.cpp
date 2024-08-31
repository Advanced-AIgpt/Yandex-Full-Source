#include "repeat.h"

#include <alice/hollywood/library/scenarios/music/common.h>
#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>

#include <alice/hollywood/library/scenarios/music/scene/common/glagol_metadata.h>
#include <alice/hollywood/library/scenarios/music/scene/common/render.h>
#include <alice/hollywood/library/scenarios/music/scene/common/structs.h>

#include <alice/megamind/protos/common/frame.pb.h>

namespace NAlice::NHollywoodFw::NMusic {

namespace {

constexpr TCommandInfo COMMAND_INFO{
    .ProductScenarioName = "player_commands",
    .Intent = "personal_assistant.scenarios.player_repeat",
    .NlgTemplate = NHollywood::NMusic::TEMPLATE_PLAYER_REPEAT,
    .ActionInfo = TCommandInfo::TActionInfo{
        .Id = "player_repeat",
        .Name = "player repeat",
        .HumanReadable = "Включается режим повтора",
    }
};

NHollywood::NMusic::ERepeatType GetRepeatType(const TRepeatModeSlot_EValue repeatMode) {
    static const THashMap<TRepeatModeSlot_EValue, NHollywood::NMusic::ERepeatType> REPEAT_TYPE_MAP{
        {TRepeatModeSlot_EValue_One, NHollywood::NMusic::RepeatTrack},
        {TRepeatModeSlot_EValue_All, NHollywood::NMusic::RepeatAll},
        {TRepeatModeSlot_EValue_None, NHollywood::NMusic::RepeatNone},
    };
    if (const auto* iter = REPEAT_TYPE_MAP.FindPtr(repeatMode)) {
        return *iter;
    }
    return NHollywood::NMusic::RepeatTrack;
}

} // namespace

TMusicScenarioScenePlayerCommandRepeat::TMusicScenarioScenePlayerCommandRepeat(const TScenario* owner)
    : TScene{owner, "player_command_repeat"}
{
}

TRetMain TMusicScenarioScenePlayerCommandRepeat::Main(const TMusicScenarioSceneArgsPlayerCommandRepeat& sceneArgs,
                                                      const TRunRequest& request,
                                                      TStorage& storage,
                                                      const TSource& source) const
{
    TScenarioRequestData requestData{.Request = request, .Storage = storage, .Source = &source};
    TScenarioStateData state{requestData};

    TCommonRenderData renderData;
    auto& nlgData = *renderData.RenderArgs.MutableNlgData();
    nlgData.SetTemplate(COMMAND_INFO.NlgTemplate.data(), COMMAND_INFO.NlgTemplate.size());
    nlgData.SetPhrase("render_result");

    // TODO(sparkle): refactor to function
    if (sceneArgs.GetCommonArgs().GetFrame().GetDisableNlg()) {
        nlgData.MutableContext()->SetNlgDisabled(true);
    }

    if (state.MusicQueue.IsGenerative()) {
        nlgData.MutableContext()->SetIsGenerative(true);
    } else if (state.MusicQueue.IsFmRadio()) {
        nlgData.MutableContext()->SetIsFmRadio(true);
        nlgData.MutableContext()->SetFmRadioName(state.MusicQueue.CurrentItem().GetTitle());
    } else {
        state.MusicQueue.RepeatPlayback(GetRepeatType(sceneArgs.GetRepeatMode()));
        *renderData.RenderArgs.AddDirectiveList()->MutableSetGlagolMetadataDirective() = BuildSetGlagolMetadataDirective(state.MusicQueue);
        state.RepeatedSkip.ResetCount();
    }

    requestData.FillAnalyticsInfo(COMMAND_INFO, state);
    renderData.FillRunFeatures(requestData);

    return TReturnValueRender(&CommonRender, renderData.RenderArgs, std::move(renderData.RunFeatures));
}

} // namespace NAlice::NHollywoodFw::NMusic
