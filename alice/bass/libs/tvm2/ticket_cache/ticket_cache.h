#pragma once

#include "ticket_cache_holder.h"

#include <util/generic/maybe.h>
#include <util/generic/noncopyable.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/system/rwlock.h>

namespace NTVM2 {

class ITicketCache : NNonCopyable::TNonCopyable {
public:
    enum class EErrorType { Logic /* "logic" */ };
    using TError = NBASS::TGenericError<EErrorType>;

public:
    virtual ~ITicketCache() = default;

    virtual TMaybe<TError> Update() = 0;

    virtual TMaybe<TString> GetTicket(TStringBuf serviceId) = 0;
    virtual TDuration SinceLastUpdate() = 0;
};

// This class *IS* thread-safe.
class TTicketCache : public ITicketCache {
public:
    using IDelegate = TTicketCacheHolder::IDelegate;

public:
    TTicketCache(IDelegate& delegate, const TVector<TString>& services);

    TMaybe<TError> Update() override;

    TMaybe<TString> GetTicket(TStringBuf serviceId) override;
    TDuration SinceLastUpdate() override;

private:
    THashMap<TString, TMaybe<TString>> Tickets;
    TInstant LastUpdate;
    TRWMutex Mutex;

    TTicketCacheHolder TicketCacheHolder;
    const TVector<TString> Services;
};
} // namespace NTVM2
