#pragma once

#include <alice/bass/libs/fetcher/fwd.h>
#include <alice/bass/libs/source_request/handle.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NBASS {

using TSocialismRequest = std::unique_ptr<IRequestHandle<TString>>;

/** Creates and starts a request for socialism API with default error type EType::SOCIALISMERROR.
 * @param[in] req is a base request (Context.GetSources().SocialApi().(Request()|AttachRequest(..))).
 * @param[in] passportUid is a user id from passport api.
 * @param[in] applicationName is a key for socialism api for which it requests oauth token.
 * @return request object which can be used to obtain a response from API.
 */
TSocialismRequest FetchSocialismToken(NHttpFetcher::TRequestPtr req, TStringBuf passportUid, TStringBuf applicationName);

} // namespace NBASS
