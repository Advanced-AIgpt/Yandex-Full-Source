#pragma once

#include <alice/bass/forms/context/fwd.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NBASS {
namespace NExternalSkill {

TString HmacSha256Hex(TStringBuf secret, TStringBuf data);

/** Generate url signature for redirector.
 * It is being calculated as a lowercased hex md5 hmac of cgi-like concatenation of key and value parameters,
 * but without cgi escaping with additional client id at the end.
 * @see https://wiki.yandex-team.ru/users/hulan/Redirector-and-Infected-services/
 */
TString SignUrlForRedirector(TStringBuf secret, TStringBuf clientId, TStringBuf url);

/** Url wrapper for redirector.
 * @see https://wiki.yandex-team.ru/users/hulan/Redirector-and-Infected-services/
 * @param[in] ctx is a context from which we obtain base SBA url, client id and secret for signing.
 * @param[in] url is an url which is going to be signed/wrapped
 * @return a wrapped and signed url
 */
TString WrapUrlWithRedirector(const TContext& ctx, TStringBuf url);

} // namespace NExternalSkill
} // namespace NBASS
