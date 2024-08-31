#include "iot.h"

#include <alice/cuttlefish/library/cuttlefish/common/edge_flags.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>

#include <alice/cuttlefish/library/protos/context_load.pb.h>

#include <util/generic/maybe.h>


namespace {

    TMaybe<NAliceProtocol::TContextLoadSmarthomeUid> TryContextLoadSmarthomeUid(
        const NVoicetech::NUniproxy2::TMessage& message
    ) {
        static constexpr TStringBuf SMARTHOME_UID_JSON_PATH =
            "event.payload.request.additional_options.quasar_auxiliary_config.alice4business.smart_home_uid";

        if (const NJson::TJsonValue* uid = message.Json.GetValueByPath(SMARTHOME_UID_JSON_PATH)) {
            NAliceProtocol::TContextLoadSmarthomeUid uidProto;

            if (uid->IsString()) {
                uidProto.SetValue(uid->GetString());
            } else if (uid->IsInteger()) {
                uidProto.SetValue(ToString(uid->GetInteger()));
            } else if (uid->IsUInteger()) {
                uidProto.SetValue(ToString(uid->GetUInteger()));
            } else {
                return Nothing();
            }

            return uidProto;
        }
        return Nothing();
    }

    TMaybe<NJson::TJsonValue> TryGetPredefinedIotConfig(const NVoicetech::NUniproxy2::TMessage& message) {
        if (const NJson::TJsonValue* jsonField = message.Json.GetValueByPath("event.payload.request.iot_config")) {
            NJson::TJsonValue iotConfig;
            iotConfig["has_predefined_iot_config"] = true;
            iotConfig["serialized_iot_config"] = jsonField->GetString();
            return iotConfig;
        }
        return Nothing();
    }

    bool TryPassPredefinedIotConfig(
        const NVoicetech::NUniproxy2::TMessage& message,
        const NAliceProtocol::TSessionContext& sessionContext,
        NAppHost::TServiceContextPtr appHostContext
    ) {
        if (sessionContext.GetUserInfo().GetUuidKind() == NAliceProtocol::TUserInfo::ROBOT) {
            if (auto x = TryGetPredefinedIotConfig(message)) {
                appHostContext->AddItem(std::move(*x), NAlice::NCuttlefish::ITEM_TYPE_PREDEFINED_IOT_CONFIG);
                return true;
            }
        }
        return false;
    }

}  // anonymous namespace


namespace NAlice::NCuttlefish::NAppHostServices {

    void SetupIotUserInfoForOwner(
        const NVoicetech::NUniproxy2::TMessage& message,
        const NAliceProtocol::TSessionContext& sessionContext,
        const NAliceProtocol::TRequestContext& /* requestContext */,
        NAppHost::TServiceContextPtr appHostContext
    ) {
        const bool hasPredefinedIotConfig = TryPassPredefinedIotConfig(message, sessionContext, appHostContext);

        if (!hasPredefinedIotConfig) {
            if (auto x = TryContextLoadSmarthomeUid(message)) {
                appHostContext->AddProtobufItem(std::move(*x), ITEM_TYPE_SMARTHOME_UID);
                appHostContext->AddFlag(EDGE_FLAG_SMARTHOME_UID);
            }
            appHostContext->AddFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_IOT_USER_INFO);
        }
    }

}  // namespace NAlice::NCuttlefish::NAppHostServices
