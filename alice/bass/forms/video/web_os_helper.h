#pragma once

#include "defs.h"

#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/video/items.h>

namespace NBASS::NVideo {

void AddWebOSLaunchAppCommandForOnboarding(TContext& ctx);

void AddWebOSLaunchAppCommandForVideoPlay(TContext& ctx, const NSc::TValue& payload);

void AddWebOSLaunchAppCommandForShowDescription(TContext& ctx, TVideoItemConstScheme item);

void AddWebOSLaunchAppCommandForShowGallery(TContext& ctx, TVideoGallery& gallery);

void AddWebOSLaunchAppCommandForShowSeasonGallery(TContext& ctx, const TVideoGallery& gallery);

void AddWebOSLaunchAppCommandForPaymentScreen(TContext& ctx, const TCandidateToPlay& candidate);

} // namespace NBASS::NVideo
