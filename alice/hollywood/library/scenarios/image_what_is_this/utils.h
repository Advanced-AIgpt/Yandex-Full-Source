#include <util/generic/strbuf.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/analytics_info.pb.h>

namespace NAlice::NHollywood::NImage {

TStringBuf CleanUrl(const TStringBuf& url);
TString GetHostname(const TStringBuf& url);
TString CutText(const TString& text, size_t len);
NAlice::NScenarios::TAnalyticsInfo::TAction CaptureModeToAnalyticsAction(const TStringBuf cameraMode, bool isStartImageRecognizer);
TString CaptureModeToString(NAlice::NScenarios::TInput::TImage::ECaptureMode captureMode);
bool IsCyrillic(const TStringBuf text);

}
