#include "rewind.h"

#include <alice/hollywood/library/scenarios/music/common.h>
#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>
#include <alice/hollywood/library/scenarios/music/scene/common/render.h>
#include <alice/hollywood/library/scenarios/music/scene/common/structs.h>

#include <alice/hollywood/library/analytics_info/util.h>

#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>

namespace NAlice::NHollywoodFw::NMusic {

namespace {

constexpr TCommandInfo COMMAND_INFO{
    .ProductScenarioName = "player_commands",
    .Intent = "personal_assistant.scenarios.player_rewind",
    .NlgTemplate = NHollywood::NMusic::TEMPLATE_PLAYER_REWIND,
    .ActionInfo = TCommandInfo::TActionInfo{
        .Id = "player_rewind",
        .Name = "player rewind",
        // HumanReadable is different on each request
    }
};

TDuration ParseRewindDuration(TStringBuf rewindTimeStr) {
    if (rewindTimeStr.Empty()) {
        return TDuration::Seconds(10); // it is the "перемотай немного вперед/назад" case
    }

    TDuration duration;
    const NJson::TJsonValue json = JsonFromString(rewindTimeStr);
    duration += TDuration::Hours(1) * json["hours"].GetDoubleRobust();
    duration += TDuration::Minutes(1) * json["minutes"].GetDoubleRobust();
    duration += TDuration::Seconds(1) * json["seconds"].GetDoubleRobust();
    return duration;
}

NScenarios::TAudioRewindDirective_EType ParseRewindType(TStringBuf rewindTypeStr, TStringBuf rewindTimeStr) {
    // "перемотай немного" case
    if (rewindTypeStr.Empty() && rewindTimeStr.Empty()) {
        return NScenarios::TAudioRewindDirective_EType_Forward;
    }

    if (rewindTypeStr == "backward") {
        return NScenarios::TAudioRewindDirective_EType_Backward;
    } else if (rewindTypeStr == "forward") {
        return NScenarios::TAudioRewindDirective_EType_Forward;
    }

    // default value
    return NScenarios::TAudioRewindDirective_EType_Absolute;
}

NScenarios::TAudioRewindDirective BuildAudioRewindDirective(const TMusicScenarioSceneArgsPlayerCommandRewind& sceneArgs) {
    const auto rewindMs = ParseRewindDuration(sceneArgs.GetTime()).MilliSeconds();
    const auto rewindType = ParseRewindType(sceneArgs.GetRewindType(), sceneArgs.GetTime());

    NScenarios::TAudioRewindDirective directive;
    directive.SetType(rewindType);
    directive.SetAmountMs(rewindMs);
    return directive;
}

TString BuildActionHumanReadable(const NScenarios::TAudioRewindDirective& directive) {
    const ui32 rewindSec = directive.GetAmountMs() / 1000;

    switch (directive.GetType()) {
        case NScenarios::TAudioRewindDirective_EType_Forward:
            return TString::Join("Перематывает вперед на ", NHollywood::TimeAmountToStr(rewindSec));
        case NScenarios::TAudioRewindDirective_EType_Backward:
            return TString::Join("Перематывает назад на ", NHollywood::TimeAmountToStr(rewindSec));
        case NScenarios::TAudioRewindDirective_EType_Absolute:
            return TString::Join("Перематывает на ", NHollywood::TimePointWhenToStr(rewindSec));
        case NScenarios::TAudioRewindDirective_EType_TAudioRewindDirective_EType_INT_MIN_SENTINEL_DO_NOT_USE_:
        case NScenarios::TAudioRewindDirective_EType_TAudioRewindDirective_EType_INT_MAX_SENTINEL_DO_NOT_USE_:
            Y_UNREACHABLE();
    }
}

}

TMusicScenarioScenePlayerCommandRewind::TMusicScenarioScenePlayerCommandRewind(const TScenario* owner)
    : TScene{owner, "player_command_rewind"}
{
}

TRetMain TMusicScenarioScenePlayerCommandRewind::Main(const TMusicScenarioSceneArgsPlayerCommandRewind& sceneArgs,
                                                      const TRunRequest& request,
                                                      TStorage& storage,
                                                      const TSource& source) const
{
    TScenarioRequestData requestData{.Request = request, .Storage = storage, .Source = &source};
    TScenarioStateData state{requestData};
    TCommonRenderData renderData;

    if (state.MusicQueue.IsGenerative() || state.MusicQueue.IsFmRadio()) {
        // render NLG, don't send directive
        auto& nlgData = *renderData.RenderArgs.MutableNlgData();
        nlgData.SetTemplate(COMMAND_INFO.NlgTemplate.data(), COMMAND_INFO.NlgTemplate.size());
        nlgData.SetPhrase("render_result");

        if (state.MusicQueue.IsGenerative()) {
            nlgData.MutableContext()->SetIsGenerative(true);
        } else {
            nlgData.MutableContext()->SetIsFmRadio(true);
            nlgData.MutableContext()->SetFmRadioName(state.MusicQueue.CurrentItem().GetTitle());
        }
    } else {
        // don't render NLG, send directive
        auto directive = BuildAudioRewindDirective(sceneArgs);

        {
            const TString actionHumanReadable = BuildActionHumanReadable(directive);
            auto commandInfo = COMMAND_INFO;
            commandInfo.ActionInfo->HumanReadable = actionHumanReadable;
            requestData.FillAnalyticsInfo(commandInfo, state);
        }

        *renderData.RenderArgs.AddDirectiveList()->MutableAudioRewindDirective() = std::move(directive);
        state.RepeatedSkip.ResetCount();
    }

    renderData.FillRunFeatures(requestData);

    return TReturnValueRender(&CommonRender, renderData.RenderArgs, std::move(renderData.RunFeatures));
}

} // namespace NAlice::NHollywoodFw::NMusic
