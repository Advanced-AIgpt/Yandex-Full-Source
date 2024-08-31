#pragma once

#include <alice/bass/forms/video/video.h>
#include <alice/bass/forms/video/billing.h>
#include <alice/bass/forms/video/items.h>
#include <alice/bass/forms/video/video_provider.h>

namespace NBASS::NVideo {

IVideoClipsProvider::TPlayResult PlayVideo(
    TVideoItemConstScheme curr,
    TVideoItemConstScheme next,
    TVideoItemConstScheme parent,
    const IVideoClipsProvider& provider, TContext& ctx,
    const TMaybe<TPlayData>& billingData,
    const TMaybe<NHttpFetcher::THandle::TRef>& vhRequest);

NVideo::IVideoClipsProvider::TPlayResult PlayVideo(const TCandidateToPlay& candidate,
                                                   const NVideo::IVideoClipsProvider& provider, TContext& ctx,
                                                   const TMaybe<NVideo::TPlayData>& billingData, const TMaybe<NHttpFetcher::THandle::TRef>& vhRequest);

TResultValue PlayVideoAndAddAttentions(const TCandidateToPlay& candidate, const NVideo::IVideoClipsProvider& provider,
                                       TContext& ctx, const TMaybe<NVideo::TPlayData>& billingData,
                                       const TMaybe<NHttpFetcher::THandle::TRef>& vhRequest);
} // namespace NBASS::NVideo
