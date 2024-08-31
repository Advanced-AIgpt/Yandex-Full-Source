#include "cachalot_mm_session.h"

#include <alice/cuttlefish/library/cuttlefish/common/edge_flags.h>
#include <alice/cuttlefish/library/cuttlefish/common/exp_flags.h>
#include <alice/cuttlefish/library/cuttlefish/common/field_getters.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>

#include <alice/cachalot/api/protos/cachalot.pb.h>
#include <alice/cuttlefish/library/protos/session.pb.h>

#include <voicetech/library/itags/itags.h>
#include <voicetech/library/settings_manager/proto/settings.pb.h>

#include <util/generic/string_hash.h>


namespace {

    NCachalotProtocol::TMegamindSessionRequest BuildMegamindSessionRequest(
        const NVoicetech::NUniproxy2::TMessage& message,
        const NAliceProtocol::TSessionContext& sessionContext,
        const bool isCrossDc
    ) {
        NCachalotProtocol::TMegamindSessionRequest megamindSessionRequest;
        auto& loadRequest = *megamindSessionRequest.MutableLoadRequest();
        loadRequest.SetUuid(NAlice::NCuttlefish::NAppHostServices::GetUuid(sessionContext));

        if (const NJson::TJsonValue* dialogId = message.Json.GetValueByPath("event.payload.header.dialog_id")) {
            loadRequest.SetDialogId(dialogId->GetString());
        }
        if (const NJson::TJsonValue* requestId = message.Json.GetValueByPath("event.payload.header.prev_req_id")) {
            loadRequest.SetRequestId(requestId->GetString());
        }
        if (isCrossDc) {
            loadRequest.SetLocation(TInstanceTags::Get().Geo);
        }

        return megamindSessionRequest;
    }

}  // anonymous namespace


namespace NAlice::NCuttlefish::NAppHostServices {

    void SetupCachalotMMSessionForOwner(
        const NVoicetech::NUniproxy2::TMessage& message,
        const NAliceProtocol::TSessionContext& sessionContext,
        const NAliceProtocol::TRequestContext& requestContext,
        NAppHost::TServiceContextPtr appHostContext
    ) {
        const bool isLoadCrossDc = (
            requestContext.HasSettingsFromManager() &&
            requestContext.GetSettingsFromManager().GetLoadMegamindSessionCrossDc()
        );
        const bool isSaveCrossDc = (
            requestContext.HasSettingsFromManager() &&
            requestContext.GetSettingsFromManager().GetCacheMegamindSessionCrossDc()
        );

        NCachalotProtocol::TMegamindSessionRequest req = BuildMegamindSessionRequest(
            message,
            sessionContext,
            isLoadCrossDc
        );
        const TStringBuf uuid = req.GetLoadRequest().GetUuid();
        appHostContext->AddBalancingHint("CACHALOT_MM_SESSION", CityHash64(uuid.data(), uuid.size()));
        appHostContext->AddProtobufItem(std::move(req), ITEM_TYPE_MEGAMIND_SESSION_REQUEST);

        // TODO(paxakor): remove after release
        appHostContext->AddFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_CACHALOT_MM_SESSION);
        appHostContext->AddFlag(EDGE_FLAG_SAVE_CONTEXT_SOURCE_CACHALOT_MM_SESSION);

        if (isLoadCrossDc) {
            appHostContext->AddFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_CACHALOT_MM_SESSION_CROSS_DC);
        } else {
            appHostContext->AddFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_CACHALOT_MM_SESSION_LOCAL_DC);
        }

        if (isSaveCrossDc) {
            appHostContext->AddFlag(EDGE_FLAG_SAVE_CONTEXT_SOURCE_CACHALOT_MM_SESSION_CROSS_DC);
        } else {
            appHostContext->AddFlag(EDGE_FLAG_SAVE_CONTEXT_SOURCE_CACHALOT_MM_SESSION_LOCAL_DC);
        }
    }

}  // namespace NAlice::NCuttlefish::NAppHostServices
