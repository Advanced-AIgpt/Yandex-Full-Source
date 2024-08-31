#pragma once

#include <alice/bass/libs/fetcher/request.h>
#include <alice/bass/util/generic_error.h>

#include <library/cpp/tvmauth/deprecated/service_context.h>
#include <library/cpp/tvmauth/version.h>

#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NTVM2 {
// This class *IS NOT* thread-safe.
class TTicketCacheHolder : NNonCopyable::TNonCopyable {
public:
    enum class EErrorType {
        BadResponse /* "bad_response" */,
        EmptyTickets /* "empty_tickets" */,
        NoRequest /* "no_request" */
    };
    using TError = NBASS::TGenericError<EErrorType>;

    struct IDelegate {
        virtual ~IDelegate() = default;

        virtual TString GetClientId() = 0;
        virtual TString GetClientSecret() = 0;
        virtual THolder<NHttpFetcher::TRequest> MakeRequest() = 0;
    };

public:
    explicit TTicketCacheHolder(IDelegate& delegate);

    // Adds service to a list of services.
    void AddService(TStringBuf id);

    // Returns ticket for a service.
    TMaybe<TString> GetTicket(TStringBuf id) const;

    // Updates tickets for all registered services. Returns true, in
    // case of success, or false, in case of failure, possibly with
    // log messages.
    //
    // NOTE: this method also may throw an exception.
    TMaybe<TError> Update();

private:
    IDelegate& Delegate;
    NTvmAuth::TServiceContext ServiceContext;
    THashMap<TString, TMaybe<TString>> Tickets;
};
} // namespace NTVM2
