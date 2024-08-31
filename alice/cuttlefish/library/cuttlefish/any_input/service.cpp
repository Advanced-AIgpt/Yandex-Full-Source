#include "service.h"

#include <alice/cuttlefish/library/cuttlefish/common/edge_flags.h>
#include <alice/cuttlefish/library/cuttlefish/common/field_getters.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>

#include <alice/cachalot/api/protos/cachalot.pb.h>
#include <alice/cuttlefish/library/protos/context_load.pb.h>
#include <alice/cuttlefish/library/protos/events.pb.h>

#include <util/string/builder.h>

namespace NAlice::NCuttlefish::NAppHostServices {

namespace {

NCachalotProtocol::TMegamindSessionRequest ConstructMegamindSessionRequest(const NAliceProtocol::TSessionContext& sessionCtx, const NAliceProtocol::TRequestContext& requestCtx) {
    NCachalotProtocol::TMegamindSessionRequest megamindSessionRequest;
    auto& loadRequest = *megamindSessionRequest.MutableLoadRequest();

    loadRequest.SetUuid(GetUuid(sessionCtx));
    const auto& header = requestCtx.GetHeader();
    if (header.HasDialogId()) {
        loadRequest.SetDialogId(header.GetDialogId());
    }
    if (header.HasPrevReqId()) {
        loadRequest.SetRequestId(header.GetPrevReqId());
    }

    return megamindSessionRequest;
}

TMaybe<NAliceProtocol::TContextLoadSmarthomeUid> TryConstructSmarthomeUid(const NAliceProtocol::TRequestContext& requestCtx) {
    const auto& additionalOptions = requestCtx.GetAdditionalOptions();
    if (additionalOptions.HasSmarthomeUid()) {
        NAliceProtocol::TContextLoadSmarthomeUid uidProto;
        uidProto.SetValue(additionalOptions.GetSmarthomeUid());
        return uidProto;
    }
    return Nothing();
}

}

void AnyInputPre(NAppHost::IServiceContext& serviceCtx, TLogContext logContext) {
    const auto sessionCtx = serviceCtx.GetOnlyProtobufItem<NAliceProtocol::TSessionContext>(ITEM_TYPE_SESSION_CONTEXT);
    const auto requestCtx = serviceCtx.GetOnlyProtobufItem<NAliceProtocol::TRequestContext>(ITEM_TYPE_REQUEST_CONTEXT);

    const auto megamindSessionRequest = ConstructMegamindSessionRequest(sessionCtx, requestCtx);
    logContext.LogEvent(NEvClass::InfoMessage(TStringBuilder() << "Megamind session request for ContextLoad: " << megamindSessionRequest));
    serviceCtx.AddProtobufItem(megamindSessionRequest, ITEM_TYPE_MEGAMIND_SESSION_REQUEST);

    if (const auto smarthomeUid = TryConstructSmarthomeUid(requestCtx)) {
        logContext.LogEvent(NEvClass::InfoMessage(TStringBuilder() << "Smarthome Uid for ContextLoad: " << *smarthomeUid));
        serviceCtx.AddProtobufItem(*smarthomeUid, ITEM_TYPE_SMARTHOME_UID);
        serviceCtx.AddFlag(EDGE_FLAG_SMARTHOME_UID);
    }
}

}  // namespace NAlice::NCuttlefish::NAppHostServices
