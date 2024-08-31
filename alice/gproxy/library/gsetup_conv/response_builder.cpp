#include <alice/gproxy/library/events/gproxy.ev.pb.h>
#include "response_builder.h"


namespace NGProxy {

void TResponseBuilder::SetGrpcResponse(
    const NJson::TJsonValue& input,
    const NGProxy::TMetadata& meta,
    const NGProxy::GSetupRequestInfo& info,
    NGProxy::GSetupResponse& resp
) const {
    Y_UNUSED(meta, info);

    if (!input.Has("response") && !input.Has("voice_response")) {
        throw std::runtime_error("no '//response' field in megamind answer");
    }

    if (!input["response"].Has("directives")) {
        throw std::runtime_error("no '//response/directives' in megamind answer");
    }

    if (input["response"].Has("ref_message_id")) {
        NEvClass::MegamindResponseParams message = NEvClass::MegamindResponseParams();
        message.SetMessageId(input["response"]["ref_message_id"].GetString());
        LogFrame->LogEvent(message);
    }

    const NJson::TJsonValue& directives = input["response"]["directives"];
    if (!directives.IsArray()) {
        throw std::runtime_error("'//response/directives' is not an array");
    }

    bool found = false;
    for (const auto& item : directives.GetArray()) {
        if (!item.IsMap()) {
            continue;
        }

        if (!item.Has("name")) {
            continue;
        }

        if (!item["name"].IsString()) {
            continue;
        }

        const TString& name = item["name"].GetString();

        if (!name.StartsWith("grpc_response")) {
            continue;
        }

        if (!item.Has("payload")) {
            continue;
        }

        if (!item["payload"].Has("grpc_response")) {
            continue;
        }

        if (!item["payload"]["grpc_response"].IsString()) {
            continue;
        }

        resp.SetData(item["payload"]["grpc_response"].GetString());

        found = true;
        break;
    }

    if (!found) {
        throw std::runtime_error("no 'grpc_response*' directive was found");
    }
}


}   // namespace NGProxy
