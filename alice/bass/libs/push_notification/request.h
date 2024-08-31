#pragma once

#include <alice/bass/libs/push_notification/scheme/scheme.sc.h>

#include <alice/bass/libs/fetcher/request.h>
#include <alice/bass/libs/globalctx/fwd.h>
#include <alice/bass/util/error.h>

#include <library/cpp/http/io/headers.h>
#include <library/cpp/scheme/domscheme_traits.h>
#include <library/cpp/scheme/scheme.h>
#include <library/cpp/scheme/util/scheme_holder.h>

#include <util/generic/strbuf.h>
#include <util/generic/variant.h>

namespace NBASS::NPushNotification {

struct TPushRequest {
    TPushRequest(const TString& name, THolder<NHttpFetcher::TRequest> req)
        : Name(name)
        , Request(std::move(req))
    {
    }

    TString Name;
    THolder<NHttpFetcher::TRequest> Request;
};

using TRequests = TVector<TPushRequest>;
using TResult = std::variant<TRequests, TError>;

using TApiScheme = NBassPushNotification::TApiRequest<TSchemeTraits>;
using TApiSchemeHolder = TSchemeHolder<TApiScheme>;

using TCallbackDataScheme = NBassPushNotification::TCallbackData<TSchemeTraits>;
using TCallbackDataSchemeHolder = TSchemeHolder<TCallbackDataScheme>;

TResult GetRequests(IGlobalContext& globalCtx, TStringBuf body);
TResult GetRequests(IGlobalContext& globalCtx, TApiSchemeHolder scheme, TCallbackDataSchemeHolder callbackData);
TResult GetRequestsLocal(IGlobalContext& globalCtx, NSc::TValue serviceData, TString service, TString event, TCallbackDataSchemeHolder callbackData);

void SendPushUnsafe(const NPushNotification::TResult& requestsVariant, TStringBuf event);

} // namespace NBASS::NPushNotification
