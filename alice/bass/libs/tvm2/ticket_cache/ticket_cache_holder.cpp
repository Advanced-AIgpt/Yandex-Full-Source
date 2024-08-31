#include "ticket_cache.h"

#include <library/cpp/scheme/scheme.h>

#include <util/datetime/base.h>
#include <util/generic/yexception.h>
#include <util/string/builder.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/string/join.h>

#include <utility>

namespace NTVM2 {
TTicketCacheHolder::TTicketCacheHolder(IDelegate& delegate)
    : Delegate(delegate)
    , ServiceContext(NTvmAuth::TServiceContext::SigningFactory(delegate.GetClientSecret())) {
}

void TTicketCacheHolder::AddService(TStringBuf id) {
    // Insert does nothing if there's an entry in Tickets with |id|.
    Tickets.insert(std::make_pair(TString{id}, TMaybe<TString>{}));
}

TMaybe<TString> TTicketCacheHolder::GetTicket(TStringBuf id) const {
    if (const auto* entry = Tickets.FindPtr(id))
        return *entry;
    return Nothing();
}

TMaybe<TTicketCacheHolder::TError> TTicketCacheHolder::Update() {
    if (Tickets.empty())
        return {};

    const TString timestamp = ToString(TInstant::Now().Seconds());

    TStringBuilder ids;
    bool first = true;
    for (auto&& [ id, ticket ] : Tickets) {
        if (!first)
            ids << ',';
        ids << id;
        first = false;
    }

    const TString signature{ServiceContext.SignCgiParamsForTvm(timestamp, ids)};

    TCgiParameters cgi;
    cgi.InsertUnescaped(TStringBuf("grant_type"), TStringBuf("client_credentials"));
    cgi.InsertUnescaped(TStringBuf("src"), Delegate.GetClientId());
    cgi.InsertUnescaped(TStringBuf("dst"), ids);
    cgi.InsertUnescaped(TStringBuf("ts"), timestamp);
    cgi.InsertUnescaped(TStringBuf("sign"), signature);

    auto request = Delegate.MakeRequest();
    if (!request)
        return TError{TError::EType::NoRequest, "Can't create request for TVM"};

    request->SetBody(cgi.Print());
    request->SetMethod("POST");
    request->SetContentType("application/x-www-form-urlencoded");

    const auto response = request->Fetch()->Wait();

    if (!response || response->IsError()) {
        auto msg = TStringBuilder() << "Failed to obtain service tickets: ";
        if (response)
            msg << response->GetErrorText();
        else
            msg << "no reply from server";
        return TError{TError::EType::BadResponse, msg};
    }

    NSc::TValue data;
    if (!NSc::TValue::FromJson(data, response->Data)) {
        return TError{TError::EType::BadResponse, TStringBuilder() << "Failed to parse server response: "
                                                                   << response->Data};
    }

    TVector<TString> failedIds;

    for (auto it = Tickets.begin(); it != Tickets.end(); ++it) {
        const auto& id = it->first;
        const TString ticket{data.TrySelect(id.Quote()).TrySelect("ticket").GetString()};
        if (ticket.empty())
            failedIds.push_back(id);
        else
            it->second = ticket;
    }

    if (!failedIds.empty()) {
        const auto msg = TStringBuilder() << "Got empty ticket(s) for id(s): [" << JoinSeq(", ", failedIds)
                                          << "], server response : " << data;
        return TError{TError::EType::EmptyTickets, msg};
    }

    return {};
}
} // namespace NTVM2
