#include "typed_semantic_frame_request.h"

#include "typed_frames.h"

#include <alice/megamind/protos/common/atm.pb.h>
#include <alice/megamind/protos/common/frame_request_params.pb.h>
#include <alice/megamind/protos/common/origin.pb.h>
#include <alice/megamind/protos/common/request_params.pb.h>

#include <alice/library/json/json.h>

#include <util/generic/yexception.h>

namespace NAlice {

TTypedSemanticFrameRequest::TTypedSemanticFrameRequest(const NScenarios::TParsedUtterance& parsedUtterance,
                                                       const bool validateAnalytics) {
    if (!parsedUtterance.HasTypedSemanticFrame() ||
        parsedUtterance.GetTypedSemanticFrame().GetTypeCase() == TTypedSemanticFrame::TypeCase::TYPE_NOT_SET)
    {
        ythrow yexception() << "invalid semantic frame";
    }
    if (validateAnalytics) {
        constexpr auto raise = [](const TStringBuf reason) {
            ythrow yexception() << "invalid semantic frame: " << reason;
        };
        if (!parsedUtterance.HasAnalytics()) {
            raise("no analytics block provided");
        }
        const auto& analytics = parsedUtterance.GetAnalytics();
        if (analytics.GetPurpose().empty()) {
            raise("empty purpose");
        }
        if (analytics.GetOrigin() == TAnalyticsTrackingModule::Undefined){
            raise("undefined origin");
        }
    }
    const auto& typedFrame = parsedUtterance.GetTypedSemanticFrame();
    SemanticFrame = MakeSemanticFrameFromTypedSemanticFrame(typedFrame);
    Utterance = parsedUtterance.GetUtterance();
    if (parsedUtterance.HasAnalytics()) {
        ProductScenario = parsedUtterance.GetAnalytics().GetProductScenario();
    }
    if (parsedUtterance.HasOrigin()) {
        Origin = parsedUtterance.GetOrigin();
    }

    DisableVoiceSession = false;
    DisableShouldListen = false;
    if (parsedUtterance.HasParams()) {
        DisableVoiceSession = parsedUtterance.GetParams().GetDisableOutputSpeech();
        DisableShouldListen = parsedUtterance.GetParams().GetDisableShouldListen();
    } else if (parsedUtterance.HasRequestParams()) {
        DisableVoiceSession = parsedUtterance.GetRequestParams().GetDisableOutputSpeech();
        DisableShouldListen = parsedUtterance.GetRequestParams().GetDisableShouldListen();
    }
}

TTypedSemanticFrameRequest::TTypedSemanticFrameRequest(const TSemanticFrameRequestData& requestData,
                                                       const bool validateAnalytics)
    : TTypedSemanticFrameRequest(
          [&requestData] {
              NScenarios::TParsedUtterance parsedUtterance;
              parsedUtterance.MutableTypedSemanticFrame()->CopyFrom(requestData.GetTypedSemanticFrame());
              parsedUtterance.MutableAnalytics()->CopyFrom(requestData.GetAnalytics());
              parsedUtterance.MutableOrigin()->CopyFrom(requestData.GetOrigin());
              parsedUtterance.MutableParams()->CopyFrom(requestData.GetParams());
              parsedUtterance.MutableRequestParams()->CopyFrom(requestData.GetRequestParams());
              return parsedUtterance;
          }(),
          validateAnalytics) {
}

TTypedSemanticFrameRequest::TTypedSemanticFrameRequest(const google::protobuf::Struct& payload,
                                                       const bool validateAnalytics)
    : TTypedSemanticFrameRequest(JsonToProto<NScenarios::TParsedUtterance>(JsonFromProto(payload),
                                                                           /* validateUtf8= */ true,
                                                                           /* ignoreUnknownFields= */ true),
                                 validateAnalytics)
{
}

} // namespace NAlice
