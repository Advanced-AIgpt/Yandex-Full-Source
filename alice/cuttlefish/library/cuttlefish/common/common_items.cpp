#include "common_items.h"
#include <util/generic/guid.h>


namespace NAlice::NCuttlefish::NAppHostServices {

using namespace NAliceProtocol;

TEventHeader CreateMessageHeader(TEventHeader::EMessageNamespace ns, TEventHeader::EMessageName n, TStringBuf refMessageId)
{
    TEventHeader header;
    header.SetNamespace(ns);
    header.SetName(n);
    header.SetMessageId(CreateGuidAsString());
    if (refMessageId) {
        header.SetRefMessageId(TString(refMessageId));
    }
    return header;
};

TDirective CreateEmptyDirective(TEventHeader::EMessageNamespace ns, TEventHeader::EMessageName n, TStringBuf refMessageId)
{
    NAliceProtocol::TDirective directive;
    *directive.MutableHeader() = CreateMessageHeader(ns, n, refMessageId);
    return directive;
}

TDirective CreateEventExceptionEx(const TString& scope, const TString& code, const TString& text, TStringBuf refMessageId)
{
    TDirective directive = CreateEmptyDirective(TEventHeader::SYSTEM, TEventHeader::EVENT_EXCEPTION, refMessageId);
    if (scope) {
        directive.MutableException()->SetScope(scope);
    }
    if (code) {
        directive.MutableException()->SetCode(code);
    }
    if (text) {
        directive.MutableException()->SetText(text);
    }
    return directive;
}

TDirective CreateInvalidAuth(TStringBuf refMessageId)
{
    TDirective directive = CreateEmptyDirective(TEventHeader::SYSTEM, TEventHeader::INVALID_AUTH, refMessageId);
    directive.MutableInvalidAuth();
    return directive;
}

TDirective CreateSynchronizeStateResponse(TString sessionId, TString guid, TStringBuf refMessageId)
{
    TDirective directive = CreateEmptyDirective(TEventHeader::SYSTEM, TEventHeader::SYNCHRONIZE_STATE_RESPONSE, refMessageId);
    TSynchronizeStateResponse* const payload = directive.MutableSyncStateResponse();
    payload->SetSessionId(std::move(sessionId));
    if (guid)
        payload->SetGuid(std::move(guid));
    return directive;
}

}  // namespace NAlice::NCuttlefish::NAppHostServices
