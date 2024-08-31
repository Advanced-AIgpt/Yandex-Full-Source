#include "render_handle.h"

#include <alice/hollywood/library/scenarios/tv_channels_efir/library/util.h>
#include <alice/hollywood/library/scenarios/tv_channels_efir/play_tv_channel/render_handle.h>
#include <alice/hollywood/library/scenarios/tv_channels_efir/show_tv_channels_gallery/render_handle.h>

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/library/video_common/hollywood_helpers/util.h>

#include <alice/library/experiments/flags.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NTvChannelsEfir {

void TTvChannelsEfirRenderHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    if (request.Input().FindSemanticFrame(SHOW_CHANNEL_BY_NAME_FRAME)
        && request.ExpFlag(NExperiments::EXPERIMENTAL_FLAG_PLAY_CHANNEL_BY_NAME)) {
        PlayTvChannelRenderHandle(ctx);
    } else if (request.Input().FindSemanticFrame(SHOW_TV_CHANNELS_GALLERY_FRAME)) {
        ShowTvChannelsGalleryRenderHandle(ctx);
    } else {
        NVideoCommon::AddIrrelevantResponse(ctx);
    }
}

} // namespace NAlice::NHollywood::NTvChannelsEfir
