#pragma once

#include <alice/hollywood/library/response/response_builder.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>

namespace NAlice::NHollywood {

enum class EFrameRedirectType {
    Client = 1 << 0, // makes client directive
    Server = 1 << 1, // makes server directive
};
Y_DECLARE_FLAGS(TFrameRedirectTypeFlags, EFrameRedirectType);
Y_DECLARE_OPERATORS_FOR_FLAGS(TFrameRedirectTypeFlags);

struct TFrameRedirectResponse {
    TMaybe<NScenarios::TMultiroomSemanticFrameDirective> Directive;
    TMaybe<NScenarios::TPushTypedSemanticFrameDirective> ServerDirective;
    TMaybe<NAlice::NScenarios::TScenarioRunResponse_TFeatures_TPlayerFeatures> PlayerFeatures;
};

// Builds a response which triggers a push with given "frame"
// to given Ya.Station device
class TFrameRedirectBuilder {
public:
    TFrameRedirectBuilder(const NAlice::TSemanticFrame& frame, // the frame to push, should have TypedSemanticFrame
                          const TStringBuf puid, // yandex user (who owns the device) id
                          const TStringBuf deviceId, // Ya.Station device id
                          const TFrameRedirectTypeFlags flags);

    TFrameRedirectBuilder& SetProductScenarioName(TStringBuf productScenarioName);
    TFrameRedirectBuilder& SetAddPlayerFeatures(bool addPlayerFeatures = true);
    TFrameRedirectResponse BuildResponse();

private:
    void FillClientDirective(NScenarios::TMultiroomSemanticFrameDirective& directive);
    void FillServerDirective(NScenarios::TPushTypedSemanticFrameDirective& directive);
    void FillRequestData(TSemanticFrameRequestData& data);

private:
    const NAlice::TSemanticFrame& Frame_;
    const TStringBuf Puid_;
    const TStringBuf DeviceId_;
    const TFrameRedirectTypeFlags Flags_;
    TMaybe<TStringBuf> ProductScenarioName_;
    bool AddPlayerFeatures_;
};

} // namespace NAlice::NHollywood
