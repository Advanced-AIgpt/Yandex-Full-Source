#include "combinator_response.h"

namespace NAlice::NMegamind {

TCombinatorResponse::TCombinatorResponse(const TCombinatorConfig& config)
    : Config_{config} {
}

void TCombinatorResponse::SetResponseProto(const NScenarios::TCombinatorResponse& proto) {
    ResponseProto_ = proto;
}

void TCombinatorResponse::SetResponseProto(NScenarios::TCombinatorResponse&& proto) {
    ResponseProto_ = std::move(proto);
}

NScenarios::TCombinatorResponse* TCombinatorResponse::ResponseProtoIfExists() {
    return ResponseProto_.Get();
}

const NScenarios::TCombinatorResponse* TCombinatorResponse::ResponseProtoIfExists() const {
    return ResponseProto_.Get();
}

const TCombinatorConfig& TCombinatorResponse::GetConfig() const {
    return Config_;
}

} // namespace NAlice::NMegamind
