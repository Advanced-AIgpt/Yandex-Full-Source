#pragma once

#include <alice/bass/forms/video/items.h>

using namespace NAlice::NVideoCommon;

namespace NBASS::NVideo {

NHttpFetcher::THandle::TRef MakeVhPlayerRequestForCandidate(const TCandidateToPlay &candidate, const TContext &ctx);

TResultValue TryPlayItemByVhResponse(const TCandidateToPlay &candidate, TContext &ctx, ESendPayPushMode sendPayPushMode,
                                          bool& isItemAvailable);

} // namespace NBASS
