#include "common.h"
#include "send_song_text.h"

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
    .Intent = "alice.music.send_song_text",
    .NlgTemplate = NHollywood::NMusic::TEMPLATE_PLAYER_SEND_SONG_TEXT,
};

// the action info for this command is filled only if push is sent
constexpr TCommandInfo::TActionInfo ACTION_INFO{
    .Id = "send_song_text",
    .Name = "send song text",
    .HumanReadable = "Отправляется ссылка на текст песни",
};

} // namespace

TMusicScenarioScenePlayerCommandSendSongText::TMusicScenarioScenePlayerCommandSendSongText(const TScenario* owner)
    : TScene{owner, "player_command_send_song_text"}
{
    RegisterRenderer(&TMusicScenarioScenePlayerCommandSendSongText::Render);
}

TRetMain TMusicScenarioScenePlayerCommandSendSongText::Main(const TMusicScenarioSceneArgsPlayerCommandSendSongText&,
                                                            const TRunRequest& request,
                                                            TStorage& storage,
                                                            const TSource& source) const
{
    TMusicScenarioRenderArgsPlayerCommandSendSongText renderArgs;

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

    return TReturnValueRender(&TMusicScenarioScenePlayerCommandSendSongText::Render,
                              renderArgs,
                              std::move(renderData.RunFeatures));
}

TRetResponse TMusicScenarioScenePlayerCommandSendSongText::Render(
    const TMusicScenarioRenderArgsPlayerCommandSendSongText& renderArgs,
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
