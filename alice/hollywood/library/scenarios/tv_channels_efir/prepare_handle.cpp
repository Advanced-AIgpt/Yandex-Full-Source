#include "prepare_handle.h"

#include <alice/hollywood/library/scenarios/tv_channels_efir/library/util.h>
#include <alice/hollywood/library/scenarios/tv_channels_efir/play_tv_channel/prepare_handle.h>
#include <alice/hollywood/library/scenarios/tv_channels_efir/show_tv_channels_gallery/prepare_handle.h>

#include <alice/library/geo/protos/user_location.pb.h>

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/library/video_common/hollywood_helpers/util.h>

#include <alice/library/experiments/flags.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NTvChannelsEfir {

void TTvChannelsEfirPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    if (request.Input().FindSemanticFrame(SHOW_CHANNEL_BY_NAME_FRAME)
        && request.ExpFlag(NExperiments::EXPERIMENTAL_FLAG_PLAY_CHANNEL_BY_NAME))
    {
        PlayTvChannelPrepareHandle(ctx);
    } else if (request.Input().FindSemanticFrame(SHOW_TV_CHANNELS_GALLERY_FRAME)) {
        ShowTvChannelsGalleryPrepareHandle(ctx);
    }
}

} // namespace NAlice::NHollywood::NTvChannelsEfir
