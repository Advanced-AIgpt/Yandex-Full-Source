#pragma once

#include "music_common.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>
#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NMusic {

void AddShotsRequest(TScenarioHandleContext& ctx, const TStringBuf userId, const TMusicQueueWrapper& mq,
                     const TMusicRequestModeInfo& musicRequestModeInfo);

void ProcessShotsResponse(TRTLogger& logger, const TStringBuf jsonStr, TMusicQueueWrapper& mq);

NAppHostHttp::THttpRequest PrepareShotFeedbackProxyRequest(NJson::TJsonValue shotFeedbackJson,
                                                  const NScenarios::TRequestMeta& meta,
                                                  const TClientInfo& clientInfo,
                                                  const bool enableCrossDc,
                                                  const TStringBuf ownerUserId,
                                                  ERequestMode requestMode,
                                                  TRTLogger& logger);

THttpProxyRequestItemPair MakeShotsLikeDislikeFeedbackProxyRequest(
    const TMusicQueueWrapper& mq, const NScenarios::TRequestMeta& meta,
    const TClientInfo& clientInfo, TRTLogger& logger, const TStringBuf userId,
    bool isLike, const bool enableCrossDc, const TMusicRequestModeInfo& musicRequestModeInfo);

bool IsThinClientShotPlaying(const TScenarioBaseRequestWrapper& request);

} // namespace NAlice::NHollywood::NMusic
