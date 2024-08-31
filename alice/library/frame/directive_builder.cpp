#include "directive_builder.h"

#include <alice/library/json/json.h>
#include <alice/library/proto/protobuf.h>

// Protobufs.
#include <alice/megamind/protos/common/atm.pb.h>

#include <library/cpp/cgiparam/cgiparam.h>

#include <util/string/cast.h>

namespace NAlice {
namespace {
const TString SD_TYPE = "server_action";
const TString SD_NAME = "@@mm_semantic_frame";
}

TTypedSemanticFrameDirectiveBuilder& TTypedSemanticFrameDirectiveBuilder::SetScenarioAnalyticsInfo(
    const TString& scenario, const TString& purpose, const TString& info)
{
    auto& atm = *TsfData_.MutableAnalytics();
    atm.SetProductScenario(scenario);
    atm.SetOrigin(TAnalyticsTrackingModule_EOrigin_Scenario);
    atm.SetPurpose(purpose);
    atm.SetOriginInfo(info);

    return *this;
}

TTypedSemanticFrameDirectiveBuilder& TTypedSemanticFrameDirectiveBuilder::SetTypedSemanticFrame(TTypedSemanticFrame tsf) {
    TsfData_.MutableTypedSemanticFrame()->Swap(&tsf);
    return *this;
}

TTypedSemanticFrame& TTypedSemanticFrameDirectiveBuilder::MutableTypedSemanticFrame() {
    return *TsfData_.MutableTypedSemanticFrame();
}

TString TTypedSemanticFrameDirectiveBuilder::BuildDialogUrl() const {
    NJson::TJsonValue directives;
    directives.AppendValue(BuildJsonDirective());
    TCgiParameters cgi;
    cgi.InsertUnescaped("directives", ToString(directives));
    return TString::Join("dialog://?", cgi.Print());
}

NJson::TJsonValue TTypedSemanticFrameDirectiveBuilder::BuildJsonDirective() const {
    NJson::TJsonValue directive;
    directive["name"] = SD_NAME;
    directive["type"] = SD_TYPE;
    directive["payload"] = JsonFromProto(TsfData_);
    return directive;
}

NSpeechKit::TDirective TTypedSemanticFrameDirectiveBuilder::BuildSpeechKitDirective() const {
    NSpeechKit::TDirective d;
    d.SetType(SD_TYPE);
    d.SetName(SD_NAME);

    *d.MutablePayload() = std::move(MessageToStruct(TsfData_));

    return d;
}

} // namespace NAlice
