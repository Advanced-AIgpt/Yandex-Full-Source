#pragma once

#include <alice/cuttlefish/library/protos/events.pb.h>
#include <alice/cuttlefish/library/protos/wsevent.pb.h>


namespace NAlice::NCuttlefish::NAppHostServices {

NAliceProtocol::TEventHeader CreateMessageHeader(
    NAliceProtocol::TEventHeader::EMessageNamespace ns,
    NAliceProtocol::TEventHeader::EMessageName n,
    TStringBuf refMessageId = {}
);

NAliceProtocol::TDirective CreateEmptyDirective(
    NAliceProtocol::TEventHeader::EMessageNamespace ns,
    NAliceProtocol::TEventHeader::EMessageName n,
    TStringBuf refMessageId = {}
);

NAliceProtocol::TDirective CreateEventExceptionEx(const TString& scope, const TString& code, const TString& error, TStringBuf refMessageId = {});
NAliceProtocol::TDirective CreateInvalidAuth(TStringBuf refMessageId = {});
NAliceProtocol::TDirective CreateSynchronizeStateResponse(TString sessionId, TString guid, TStringBuf refMessageId = {});

}  // namespace NAlice::NCuttlefish::NAppHostServices
