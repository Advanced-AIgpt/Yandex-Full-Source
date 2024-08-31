#include "vh_prepare_handle.h"
#include "musical_clips_defs.h"
#include "utils.h"

#include <alice/bass/forms/tv/defs.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/scenarios/music/util/music_proxy_request.h>
#include <alice/library/network/headers.h>
#include <alice/library/video_common/frontend_vh_helpers/frontend_vh_requests.h>
#include <alice/library/video_common/hollywood_helpers/util.h>

namespace NAlice::NHollywood::NMusicalClips {

void TVhPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioApplyRequestWrapper applyRequest{requestProto, ctx.ServiceCtx};

    NScenarios::TScenarioRunRequest requestRunProto;
    auto baseRequest = applyRequest.Proto().GetBaseRequest();
    *requestRunProto.MutableBaseRequest() = std::move(baseRequest);
    auto input = applyRequest.Proto().GetInput();
    *requestRunProto.MutableInput() = std::move(input);
    const TScenarioRunRequestWrapper request{requestRunProto, ctx.ServiceCtx};

    for (int trackNumber = 0; trackNumber < 5; trackNumber++) {
        TString responseName = MakeNumberedName(MUSIC_CLIP_RESPONSE_ITEM, trackNumber);
        TString requestName = MakeNumberedName(FRONTEND_VH_PLAYER_REQUEST_ITEM, trackNumber);
        const TMaybe<NJson::TJsonValue> searchClipsResponse = RetireHttpResponseJsonMaybe(
            ctx,
            responseName,
            NMusic::MUSIC_REQUEST_RTLOG_TOKEN_ITEM
        );
        if (!searchClipsResponse.Defined()) {
            continue;
        }

        TString trackId = (*searchClipsResponse)["result"]["id"].GetStringRobust();
        for (const auto& clip: (*searchClipsResponse)["result"]["videos"].GetArray()) {
            TString provider = clip["provider"].GetStringRobust();
            if (!clip.Has("providerVideoId"))
                continue;

            if (provider != "yandex") {
                continue;
            }

            TString requestedItemId = clip["providerVideoId"].GetStringRobust();
            LOG_INFO(ctx.Ctx.Logger()) << "FrontendVhPlayerRequest create for providerVideoId: "  << requestedItemId << ", trackId: " << trackId;

            if (!requestedItemId.empty()) {
                const THttpProxyRequest vhRequest = NVideoCommon::PrepareFrontendVhPlayerRequest(TString{requestedItemId}, request, ctx);
                AddHttpRequestItems(ctx, vhRequest, requestName, FRONTEND_VH_PLAYER_REQUEST_RTLOG_TOKEN_ITEM);
                break;
            }
        }
    }
}

} // namespace NAlice::NHollywood::NMusicalClips
