#pragma once

#include <alice/megamind/library/scenarios/config_registry/config_registry.h>

#include <alice/megamind/protos/scenarios/combinator_response.pb.h>

#include <util/generic/maybe.h>
#include <util/generic/noncopyable.h>

namespace NAlice::NMegamind {

class TCombinatorResponse : public NNonCopyable::TMoveOnly {
public:

    explicit TCombinatorResponse(const TCombinatorConfig& config);

    void SetResponseProto(const NScenarios::TCombinatorResponse& proto);
    void SetResponseProto(NScenarios::TCombinatorResponse&& proto);
    NScenarios::TCombinatorResponse* ResponseProtoIfExists();
    const NScenarios::TCombinatorResponse* ResponseProtoIfExists() const;
    const TCombinatorConfig& GetConfig() const;

private:
    TCombinatorConfig Config_;
    TMaybe<NScenarios::TCombinatorResponse> ResponseProto_;
};

} // namespace NAlice::NMegamind
