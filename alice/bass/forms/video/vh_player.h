#pragma once

#include <alice/bass/forms/video/utils.h>
#include <alice/bass/setup/setup.h>
#include <alice/library/video_common/vh_player.h>

using namespace NAlice::NVideoCommon;

namespace NBASS::NVideo {

NHttpFetcher::THandle::TRef CreateVhPlayerRequest(const NBASS::TContext& ctx, const TStringBuf itemId);
TMaybe<TVhPlayerData> GetVhPlayerDataByVhPlayerRequest(const NHttpFetcher::THandle::TRef& vhRequest);
NBASS::NVideo::TPlayVideoCommandData GetPlayCommandData(const NBASS::TContext& ctx, const TVhPlayerData& vhPlayerData);

NBASS::NVideo::TVideoItem MakeSchemeVideoItem(const TVhPlayerData& vhPlayerData);

} // NBASS::NVideo
