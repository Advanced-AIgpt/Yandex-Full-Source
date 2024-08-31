#pragma once

#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/megamind/protos/speechkit/directives.pb.h>

#include <library/cpp/json/json_value.h>

#include <util/generic/string.h>

namespace NAlice {

class TTypedSemanticFrameDirectiveBuilder {
public:
    TTypedSemanticFrameDirectiveBuilder& SetScenarioAnalyticsInfo(const TString& scenario, const TString& purpose, const TString& info);
    TTypedSemanticFrameDirectiveBuilder& SetTypedSemanticFrame(TTypedSemanticFrame tsf);

    TTypedSemanticFrame& MutableTypedSemanticFrame();

    TString BuildDialogUrl() const;
    NJson::TJsonValue BuildJsonDirective() const;

    NSpeechKit::TDirective BuildSpeechKitDirective() const;

private:
    TSemanticFrameRequestData TsfData_;
};

} // namespace NAlice
