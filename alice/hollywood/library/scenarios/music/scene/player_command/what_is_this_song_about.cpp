#include "common.h"
#include "what_is_this_song_about.h"

#include <alice/hollywood/library/scenarios/music/common.h>
#include <alice/hollywood/library/scenarios/music/scene/common/analytics_info.h>
#include <alice/hollywood/library/scenarios/music/scene/common/render.h>
#include <alice/hollywood/library/scenarios/music/scene/common/scenario_meta_processor.h>

#include <alice/hollywood/library/framework/core/codegen/gen_server_directives.pb.h>
#include <alice/hollywood/library/response/push.h>

#include <alice/megamind/protos/scenarios/push.pb.h>

namespace NAlice::NHollywoodFw::NMusic {

namespace {

constexpr TCommandInfo COMMAND_INFO{
    .ProductScenarioName = "player_commands",
    .Intent = "alice.music.what_is_this_song_about",
    .NlgTemplate = NHollywood::NMusic::TEMPLATE_PLAYER_WHAT_IS_THIS_SONG_ABOUT,
};

// the action info for this command is filled only if push is sent
constexpr TCommandInfo::TActionInfo ACTION_INFO{
    .Id = "what_is_this_song_about",
    .Name = "what is this song about",
    .HumanReadable = "Отправляется ссылка на текст песни",
};

} // namespace

TMusicScenarioScenePlayerCommandWhatIsThisSongAbout::TMusicScenarioScenePlayerCommandWhatIsThisSongAbout(const TScenario* owner)
    : TScene{owner, "player_command_what_is_this_song_about"}
{
    RegisterRenderer(&TMusicScenarioScenePlayerCommandWhatIsThisSongAbout::Render);
}

TRetMain TMusicScenarioScenePlayerCommandWhatIsThisSongAbout::Main(const TMusicScenarioSceneArgsPlayerCommandWhatIsThisSongAbout&,
                                                                   const TRunRequest& request,
                                                                   TStorage& storage,
                                                                   const TSource& source) const
{
    TMusicScenarioRenderArgsPlayerCommandWhatIsThisSongAbout renderArgs;

    const auto currentItemProcessor = [&renderArgs](TMusicScenarioRenderArgsCommon& commonRenderArgs,
                                                    const NHollywood::NMusic::TQueueItem& currentItem)
    {
        if (currentItem.GetTrackInfo().GetLyricsInfo().GetHasAvailableTextLyrics()) {
            commonRenderArgs.MutableNlgData()->MutableContext()->SetPushSent(true);
            renderArgs.MutableSendPushDirectiveArgs()->SetCurrentItemTrackId(currentItem.GetTrackId());
        }
    };

    TScenarioRequestData requestData{.Request = request, .Storage = storage, .Source = &source};
    TCommonRenderData renderData = TScenarioMetaProcessor{requestData}
        .SetCommandInfo(COMMAND_INFO)
        .SetCurrentItemProcessor(currentItemProcessor)
        .Process();
    *renderArgs.MutableCommonArgs() = std::move(renderData.RenderArgs);

    return TReturnValueRender(&TMusicScenarioScenePlayerCommandWhatIsThisSongAbout::Render,
                              renderArgs,
                              std::move(renderData.RunFeatures));
}

TRetResponse TMusicScenarioScenePlayerCommandWhatIsThisSongAbout::Render(
    const TMusicScenarioRenderArgsPlayerCommandWhatIsThisSongAbout& renderArgs,
    TRender& render) const
{
    DoCommonRender(renderArgs.GetCommonArgs(), render);

    if (renderArgs.HasSendPushDirectiveArgs()) {
        const TString title = render.RenderPhrase(COMMAND_INFO.NlgTemplate,
                                                  RENDER_SONG_TEXT_PUSH_TITLE,
                                                  /* nlgContext = */ google::protobuf::Empty()).Text;

        const TString text = render.RenderPhrase(COMMAND_INFO.NlgTemplate,
                                                 RENDER_SONG_TEXT_PUSH_TEXT,
                                                 /* nlgContext = */ google::protobuf::Empty()).Text;

        const TStringBuf trackId = renderArgs.GetSendPushDirectiveArgs().GetCurrentItemTrackId();
        const TString uri = GenerateSendSongPushUri(trackId);

        NHollywood::TPushDirectiveBuilder{title, text, uri, SONG_TEXT_PUSH_TAG}
            .SetThrottlePolicy(SONG_TEXT_PUSH_POLICY)
            .SetAnalyticsAction(TString{ACTION_INFO.Id}, TString{ACTION_INFO.Name}, TString{ACTION_INFO.HumanReadable})
            .BuildTo(render);
    }

    return TReturnValueSuccess();
}

} // namespace NAlice::NHollywoodFw::NMusic
