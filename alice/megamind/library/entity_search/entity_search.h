#pragma once

#include "response.h"

#include <alice/bass/libs/fetcher/request.h>
#include <alice/megamind/library/sources/request.h>
#include <alice/megamind/library/speechkit/request.h>
#include <alice/megamind/library/util/status.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NAlice {

TSourcePrepareStatus CreateEntitySearchRequest(const NJson::TJsonValue& begemotResponse, const TSpeechKitRequest& skr, NHttpFetcher::IRequestBuilder& request);
TErrorOr<TEntitySearchResponse> ParseEntitySearchResponse(const TString& content);

} // namespace NAlice
