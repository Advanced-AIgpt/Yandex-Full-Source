#pragma once

#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/music/proto/fast_data.pb.h>
#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>
#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>
#include <alice/library/logger/logger.h>

namespace NAlice::NHollywood::NMusic {

void FillMusicAnswer(TNlgData& nlgData, const TScenarioBaseRequestWrapper& request, const TQueueItem& currentItem, const TContentId& contentId);

bool TryFillMusicAnswer(TNlgData& nlgData, const TScenarioBaseRequestWrapper& request, const TMusicQueueWrapper& mq);

bool TryAnnounceTrack(TRTLogger& logger, const NHollywood::TScenarioApplyRequestWrapper& request,
                      TApplyResponseBuilder& builder, const TMusicQueueWrapper& mq, const TMusicContext& mCtx,
                      const TMusicShotsFastData& shots, IRng& rng);

} // namespace NAlice::NHollywood::NMusic
