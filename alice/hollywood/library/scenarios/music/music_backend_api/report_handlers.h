// TODO(vitvlkv): Better rename this module to report_and_feedback_handlers.h
#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/scenarios/music/common.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_common.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>
#include <alice/hollywood/library/scenarios/music/music_request_builder/music_request_builder.h>

namespace NAlice::NHollywood::NMusic {

const TString GENERATIVE_FEEDBACK_TYPE_TIMESTAMP_LIKE = "timestampLike";
const TString GENERATIVE_FEEDBACK_TYPE_TIMESTAMP_DISLIKE = "timestampDislike";
const TString GENERATIVE_FEEDBACK_TYPE_TIMESTAMP_SKIP = "timestampSkip";
const TString GENERATIVE_FEEDBACK_TYPE_STREAM_PLAY = "streamPlay";

std::unique_ptr<NScenarios::TScenarioCommitResponse> MakeCommitSuccessResponse();

std::variant<THttpProxyRequestItemPairs, NScenarios::TScenarioCommitResponse>
MakeMusicReportRequest(const NScenarios::TRequestMeta& meta, const TClientInfo& clientInfo,
                       TRTLogger& logger, const NHollywood::TScenarioApplyRequestWrapper& runRequest);

THttpProxyRequestItemPair MakeTimestampGenerativeFeedbackProxyRequest(const TMusicQueueWrapper& mq,
    TAtomicSharedPtr<IRequestMetaProvider> metaProvider, const TClientInfo& clientInfo, TRTLogger& logger,
    const NHollywood::TScenarioApplyRequestWrapper& applyRequest, const TString& type,
    const TMusicRequestModeInfo& musicRequestModeInfo);

NAppHostHttp::THttpRequest PrepareLikeDislikeRadioFeedbackProxyRequest(
    const TMusicQueueWrapper& mq, const NScenarios::TRequestMeta& meta,
    const TClientInfo& clientInfo, TRTLogger& logger,
    const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
    const TScenarioState& scState, const bool isLike,
    const bool enableCrossDc);

} // namespace NAlice::NHollywood::NMusic
