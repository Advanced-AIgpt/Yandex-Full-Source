#include "contacts.h"

#include <alice/cuttlefish/library/cuttlefish/common/edge_flags.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/protos/context_load.pb.h>

namespace {

    bool TrySetupPredefinedContacts(
        const NVoicetech::NUniproxy2::TMessage& message,
        NAppHost::TServiceContextPtr appHostContext
    ) {
        if (appHostContext->HasProtobufItem(NAlice::NCuttlefish::ITEM_TYPE_PREDEFINED_CONTACTS)) {
            // Hack to eliminate second call of the same call.
            // TODO (rayz): get rid of contacts-json.
            return true;
        }
        if (const NJson::TJsonValue* val = message.Json.GetValueByPath("event.payload.request.predefined_contacts")) {
            if (val->IsString()) {
                NAliceProtocol::TContextLoadPredefinedContacts proto;
                proto.SetValue(val->GetString());
                appHostContext->AddProtobufItem(std::move(proto), NAlice::NCuttlefish::ITEM_TYPE_PREDEFINED_CONTACTS);

                // TODO: remove following edge expression.
                appHostContext->AddFlag(NAlice::NCuttlefish::EDGE_FLAG_PREDEFINED_CONTACTS);
                return true;
            }
        }
        return false;
    }

}  // anonymous namespace


namespace NAlice::NCuttlefish::NAppHostServices {

    void SetupContactsJsonForOwner(
        const NVoicetech::NUniproxy2::TMessage& message,
        const NAliceProtocol::TSessionContext& /* sessionContext */,
        const NAliceProtocol::TRequestContext& /* requestContext */,
        NAppHost::TServiceContextPtr appHostContext
    ) {
        if (!TrySetupPredefinedContacts(message, appHostContext)) {
            appHostContext->AddFlag(EDGE_FLAG_CONTACTS_JSON);  // TODO: remove this line after next release (uniproxy-147).
            appHostContext->AddFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_CONTACTS_JSON);
        }
    }

    void SetupContactsProtoForOwner(
        const NVoicetech::NUniproxy2::TMessage& message,
        const NAliceProtocol::TSessionContext& /* sessionContext */,
        const NAliceProtocol::TRequestContext& /* requestContext */,
        NAppHost::TServiceContextPtr appHostContext
    ) {
        if (!TrySetupPredefinedContacts(message, appHostContext)) {
            appHostContext->AddFlag(EDGE_FLAG_CONTACTS_PROTO);  // TODO: remove this line after next release (uniproxy-147).
            appHostContext->AddFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_CONTACTS_PROTO);
        }
    }

}  // namespace NAlice::NCuttlefish::NAppHostServices
