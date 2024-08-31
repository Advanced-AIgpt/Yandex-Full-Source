#include "request_builder.h"

#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/json/json_reader.h>


namespace NGProxy {

void TRequestBuilder::SetGrpcRequest(NJson::TJsonValue& output, const NGProxy::TMetadata& meta, const NGProxy::GSetupRequestInfo& info, const NGProxy::GSetupRequest& req) {
    Y_UNUSED(meta);

    NJson::TJsonValue typedFrame;
    if (!NJson::ReadJsonTree(req.GetData(), &typedFrame, false)) {
        throw std::runtime_error("failed to parse semantic frame representation");
    }

    NJson::TJsonValue& event = output["request"]["event"];

    event["type"] = "server_action";
    event["name"] = "@@mm_semantic_frame";

    NJson::TJsonValue payload;
    NJson::TJsonValue frame;

    frame[info.GetSemanticFrameName()] = std::move(typedFrame);

    payload["typed_semantic_frame"] = std::move(frame);

    NJson::TJsonValue analytics;
    analytics["origin"] = "Scenario";
    analytics["purpose"] = "callback_directive";

    payload["analytics"] = std::move(analytics);

    event["payload"] = std::move(payload);
}

}   // namespace NGProxy
