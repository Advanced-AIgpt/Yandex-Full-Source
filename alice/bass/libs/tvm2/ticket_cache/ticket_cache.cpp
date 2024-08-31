#include "ticket_cache.h"

#include <util/generic/yexception.h>
#include <util/stream/output.h>

namespace NTVM2 {
TTicketCache::TTicketCache(IDelegate& delegate, const TVector<TString>& services)
    : TicketCacheHolder(delegate)
    , Services(services) {
    for (const auto& service : Services)
        TicketCacheHolder.AddService(service);
}

TMaybe<TString> TTicketCache::GetTicket(TStringBuf serviceId) {
    TReadGuard guard(Mutex);
    if (const auto* ticket = Tickets.FindPtr(serviceId))
        return *ticket;
    return Nothing();
}

TDuration TTicketCache::SinceLastUpdate() {
    TReadGuard guard(Mutex);
    return TInstant::Now() - LastUpdate;
}

TMaybe<TTicketCache::TError> TTicketCache::Update() try {
    THashMap<TString, TMaybe<TString>> tickets;

    const auto error = TicketCacheHolder.Update();
    for (const auto& service : Services) {
        if (const auto ticket = TicketCacheHolder.GetTicket(service))
            tickets[service] = *ticket;
    }

    {
        TWriteGuard guard(Mutex);
        Tickets.swap(tickets);
        LastUpdate = TInstant::Now();
    }

    if (error) {
        return TError{TError::EType::Logic, TStringBuilder() << "Failed to update TVM2.0 service tickets: "
                                                             << error->Msg};
    }

    return {};
} catch (const yexception& e) {
    return TError{TError::EType::Logic, TStringBuilder() << "Exception while updating TVM 2 service tickets: "
                                                         << e.what()};
}
} // namespace NTVM2

template <>
void Out<NTVM2::TTicketCache::TError>(IOutputStream& os, const NTVM2::TTicketCache::TError& error) {
    os << "TTicketCache::TError [" << error.Type << ", " << error.Msg << "]";
}
