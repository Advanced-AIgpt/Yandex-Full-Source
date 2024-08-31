#pragma once

#include "fwd.h"

#include <alice/bass/forms/context/fwd.h>
#include <alice/bass/forms/external_skill_recommendation/enums.h>
#include <alice/bass/libs/fetcher/neh.h>

#include <util/generic/fwd.h>

namespace NBASS::NExternalSkill {

TString ConstructLogoUrl(TStringBuf look, TStringBuf prefix, TStringBuf avatarId, const TContext& ctx);

NHttpFetcher::TRequestPtr PrepareRequest(TContext& ctx, EServiceRequestCard cardName, NHttpFetcher::IMultiRequest::TRef multiRequest = nullptr);
bool ParseResponse(TContext& ctx, NHttpFetcher::TResponse::TConstRef response, TServiceResponse& answer);
bool TryGetRecommendationsFromService(TContext& ctx, EServiceRequestCard cardName, TServiceResponse& answer);

}

