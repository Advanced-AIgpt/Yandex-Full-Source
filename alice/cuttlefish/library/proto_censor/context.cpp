#include "context.h"

#include <alice/cachalot/api/protos/cachalot.pb.h>
#include <alice/megamind/protos/common/iot.pb.h>
#include <alice/protos/data/contacts.pb.h>

#include <apphost/lib/proto_answers/http.pb.h>

#include <util/string/builder.h>


void NAlice::NCuttlefish::Censore(NAliceProtocol::TContextLoadResponse& r) {
    if (r.HasUserTicket()) {
        r.SetUserTicket("<censored>");
    }
    if (r.HasMegamindSessionResponse() && r.GetMegamindSessionResponse().HasMegamindSessionLoadResp()) {
        r.ClearMegamindSessionResponse();
        r.MutableMegamindSessionResponse()->MutableMegamindSessionLoadResp()->SetData("<censored>");
    }
    if (r.HasMementoResponse()) {
        r.MutableMementoResponse()->SetContent("<censored-binary-data>");
    }
    if (r.HasQuasarIotResponse()) {
        r.MutableQuasarIotResponse()->SetContent("<censored-binary-data>");
    }
    if (r.HasIoTUserInfo()) {
        r.ClearIoTUserInfo();
        r.MutableIoTUserInfo()->SetRawUserInfo("<censored>");
    }
    if (r.HasContactsResponse()) {
        r.MutableContactsResponse()->SetContent("<censored-json-data>");
    }
    if (r.HasContactsProto()) {
        TString msg = TStringBuilder() << "<censored-" << r.GetContactsProto().GetContacts().size() << "-records>";
        r.ClearContactsProto();
        r.MutableContactsProto()->SetLookupKeyMapSerialized(std::move(msg));
    }
}
