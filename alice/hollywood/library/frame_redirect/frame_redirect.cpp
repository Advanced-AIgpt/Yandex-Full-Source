#include "frame_redirect.h"

#include <alice/megamind/protos/common/atm.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>

namespace NAlice::NHollywood {

namespace {

const TString MULTIROOM_REDIRECT_ANALYTICS_PURPOSE = "multiroom_redirect";
constexpr uint MULTIROOM_REDIRECT_DIRECTIVE_TTL = 5; // may be pushed to builder methods

} // anonymous namespace

TFrameRedirectBuilder::TFrameRedirectBuilder(const NAlice::TSemanticFrame& frame,
                                             const TStringBuf puid,
                                             const TStringBuf deviceId,
                                             const TFrameRedirectTypeFlags flags)
    : Frame_{frame}
    , Puid_{puid}
    , DeviceId_{deviceId}
    , Flags_{flags}
    , AddPlayerFeatures_{false}
{
    Y_ENSURE(Flags_);
}

TFrameRedirectBuilder& TFrameRedirectBuilder::SetProductScenarioName(const TStringBuf productScenarioName) {
    ProductScenarioName_ = productScenarioName;
    return *this;
}

TFrameRedirectBuilder& TFrameRedirectBuilder::SetAddPlayerFeatures(bool addPlayerFeatures) {
    AddPlayerFeatures_ = addPlayerFeatures;
    return *this;
}

TFrameRedirectResponse TFrameRedirectBuilder::BuildResponse() {
    TFrameRedirectResponse response;
    if (Flags_ & EFrameRedirectType::Client) {
        FillClientDirective(response.Directive.ConstructInPlace());
    }
    if (Flags_ & EFrameRedirectType::Server) {
        FillServerDirective(response.ServerDirective.ConstructInPlace());
    }
    if (AddPlayerFeatures_) {
        response.PlayerFeatures.ConstructInPlace().SetRestorePlayer(true);
    }
    return response;
}

void TFrameRedirectBuilder::FillClientDirective(NScenarios::TMultiroomSemanticFrameDirective& directive) {
    directive.SetDeviceId(TString{DeviceId_});
    FillRequestData(*directive.MutableBody());
}

void TFrameRedirectBuilder::FillServerDirective(NScenarios::TPushTypedSemanticFrameDirective& directive) {
    directive.SetPuid(TString{Puid_});
    directive.SetDeviceId(TString{DeviceId_});
    directive.SetTtl(MULTIROOM_REDIRECT_DIRECTIVE_TTL);
    FillRequestData(*directive.MutableSemanticFrameRequestData());
}

void TFrameRedirectBuilder::FillRequestData(TSemanticFrameRequestData& data) {
    if (ProductScenarioName_.Defined()) {
        auto& analytics = *data.MutableAnalytics();
        analytics.SetProductScenario(TString{*ProductScenarioName_});
        analytics.SetOrigin(TAnalyticsTrackingModule_EOrigin_Scenario);
        analytics.SetPurpose(MULTIROOM_REDIRECT_ANALYTICS_PURPOSE);
    }

    if (Frame_.HasTypedSemanticFrame()) {
        const auto& tsf = Frame_.GetTypedSemanticFrame();
        data.MutableTypedSemanticFrame()->CopyFrom(tsf);
    }
}

} // namespace NAlice::NHollywood
