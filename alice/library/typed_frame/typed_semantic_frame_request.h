#pragma once

#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/megamind/protos/common/origin.pb.h>
#include <alice/megamind/protos/scenarios/frame.pb.h>

#include <google/protobuf/struct.pb.h>

#include <util/generic/string.h>
#include <util/generic/maybe.h>

namespace NAlice {

struct TTypedSemanticFrameRequest {
    explicit TTypedSemanticFrameRequest(const NScenarios::TParsedUtterance& parsedUtterance, bool validateAnalytics = true);
    explicit TTypedSemanticFrameRequest(const TSemanticFrameRequestData& requestData, bool validateAnalytics = true);
    explicit TTypedSemanticFrameRequest(const google::protobuf::Struct& payload, bool validateAnalytics = true);

    TSemanticFrame SemanticFrame;
    TString Utterance;
    TString ProductScenario;
    TMaybe<TOrigin> Origin;
    bool DisableVoiceSession;
    bool DisableShouldListen;
};

class IFrameRequestProcessor {
public:
    virtual ~IFrameRequestProcessor() = default;
    virtual void Process(const TTypedSemanticFrameRequest& frameRequest) = 0;
};

} // namespace NAlice
