#include "player_setup.h"

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/mordovia_video_selection/util.h>
#include <alice/hollywood/library/scenarios/tv_channels_efir/library/util.h>

#include <alice/library/network/headers.h>
#include <alice/library/parsed_user_phrase/parsed_sequence.h>
#include <alice/library/video_common/frontend_vh_helpers/frontend_vh_requests.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NTvChannelsEfir {

namespace {

bool IsMatchingChannelName(const TString& formName, const TString& resultName) {
    NParsedUserPhrase::TParsedSequence query(formName);
    NParsedUserPhrase::TParsedSequence document(resultName);

    // constant is happily borrowed from item_selector
    static constexpr float matchingThreshold = 0.47;

    float score = (query.Match(document) + document.Match(query)) / 2;

    return score > matchingThreshold;
}

TString GetChannelUUID(const NJson::TJsonValue& response, const TString& channelName) {
    for (const auto& doc : response["clips"].GetArray()) {
        const TString& title = doc["clear_title"].GetString();
        const TString& uuid = doc["vh_uuid"].GetString();
        if (title.empty() || uuid.empty()) {
            continue;
        }
        if (IsMatchingChannelName(channelName, title)) {
            return uuid;
        }
    }

    return TString{};
}

} // namespace

void TPlayTvChannelPlayerSetup::Do(TScenarioHandleContext& ctx) const {
    const auto videoResultResponse =
        RetireHttpResponseJsonMaybe(ctx, VIDEO_RESULT_RESPONSE_ITEM, VIDEO_RESULT_REQUEST_RTLOG_TOKEN_ITEM, /* logBody= */ false);
    if (!videoResultResponse) {
        return;
    }

    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    const TString channelName = GetChannelName(request);

    const TString channelUUID = GetChannelUUID(*videoResultResponse, channelName);

    if (channelUUID.Empty()) {
        return;
    }

    const auto vhRequest = NVideoCommon::PrepareFrontendVhPlayerRequest(channelUUID, request, ctx);
    AddHttpRequestItems(ctx, vhRequest, VH_PLAYER_REQUEST_ITEM, VH_PLAYER_REQUEST_RTLOG_TOKEN_ITEM);
}

} // namespace NAlice::NHollywood::NTvChannelsEfir
