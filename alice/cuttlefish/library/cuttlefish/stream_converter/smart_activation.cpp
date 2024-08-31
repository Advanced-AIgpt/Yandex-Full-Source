#include "smart_activation.h"

#include <alice/cuttlefish/library/cuttlefish/stream_converter/rms_converter/converter.h>

#include <alice/cuttlefish/library/experiments/flags_json.h>
#include <alice/cuttlefish/library/cuttlefish/common/exp_flags.h>
#include <alice/cachalot/api/protos/cachalot.pb.h>

#include <alice/library/json/json.h>

#include <voicetech/library/idl/log/events.ev.pb.h>
#include <voicetech/library/messages/build.h>
#include <voicetech/library/messages/message.h>
#include <voicetech/library/messages/message_to_wsevent.h>
#include <voicetech/library/settings_manager/proto/settings.pb.h>

#include <google/protobuf/timestamp.pb.h>

#include <library/cpp/protobuf/interop/cast.h>

#include <util/string/builder.h>


using namespace NJson;
using namespace NVoicetech::NUniproxy2;

namespace {

    void AppendDescription(NAliceProtocol::TWsEvent& wsEvent, const TString& extraDescription) {
        wsEvent.MutableHeader()->SetDescription(wsEvent.GetHeader().GetDescription() + extraDescription);
    }

    uint64_t TimestampMilliSeconds(const google::protobuf::Timestamp& ts) {
        return NProtoInterop::CastFromProto(ts).MilliSeconds();
    }

}  // anonymous namespace


void NAlice::NCuttlefish::NAppHostServices::FillSmartActivation(
    NCachalotProtocol::TActivationAnnouncementRequest& req,
    const NAliceProtocol::TRequestContext& requestContext,
    const TString& smartActivationUserId,
    const TString& smartActivationDeviceId,
    const TString& smartActivationDeviceModel,
    TLogContext* logContext
) {
    Y_ENSURE(logContext != nullptr);

    auto& info = *req.MutableInfo();
    info.SetUserId(smartActivationUserId);
    info.SetDeviceId(smartActivationDeviceId);
    *info.MutableActivationAttemptTime() = requestContext.GetCreatingTimestamp();
    info.MutableSpotterFeatures()->SetValidated(false);

    if (requestContext.HasAdditionalOptions()) {
        const auto& opts = requestContext.GetAdditionalOptions();
        if (opts.HasSpotterFeatures()) {
            double averageRms = requestContext.HasSettingsFromManager()
                ? NRmsConverter::CalcAvgRMS(opts.GetSpotterFeatures(), smartActivationDeviceModel, requestContext.GetSettingsFromManager().GetRmsPerDeviceConfig())
                : NRmsConverter::CalcAvgRMS(opts.GetSpotterFeatures(), smartActivationDeviceModel);

            info.MutableSpotterFeatures()->SetAvgRMS(averageRms);
        }
    }

    if (
        auto val = NExpFlags::GetExperimentValue(requestContext, "cachalot_activation_freshness_delta_milliseconds");
        val.Defined()
    ) {
        int freshnessDeltaMilliSeconds = 0;
        if (TryFromString<int>(val.GetRef(), freshnessDeltaMilliSeconds)) {
            if (freshnessDeltaMilliSeconds) {
                req.SetFreshnessDeltaMilliSeconds(freshnessDeltaMilliSeconds);
            }
        } else {
            logContext->LogEventErrorCombo<NEvClass::TErrorMessage>(
                TStringBuilder() << "fail extract integer from cachalot_activation_freshness_delta_milliseconds value=" << val.GetRef()
            );
        }
    }
}

NAliceProtocol::TWsEvent NAlice::NCuttlefish::NAppHostServices::BuildSpotterValidation(
    bool& validationResult,
    const NCachalotProtocol::TActivationFinalResponse& smartActivationResponse,
    const TMaybe<NCachalotProtocol::TActivationLog>& smartActivationLog,
    const TMaybe<bool>& asrValid,
    const TString& refMessageId
) {
    TJsonValue jsonSV;
    auto& payload = BuildDirective(jsonSV, "Spotter", "Validation", refMessageId);
    int valid = 1;
    if (asrValid.Defined() && !asrValid.GetRef()) {
        valid = 0;
    }
    payload["valid"] = valid;
    static const TString result0 = " result=0";
    static const TString result1 = " result=1";
    TString eventDescriptionSuffix = result1;
    validationResult = true;
    if (smartActivationResponse.HasActivationAllowed() && !smartActivationResponse.GetActivationAllowed()) {
        validationResult = false;
        eventDescriptionSuffix = result0;
    }
    payload["result"] = validationResult ? 1 : 0;  // main response value
    payload["canceled_cause_of_multiactivation"] = validationResult ? 0 : 1;
    payload["allow_activation_by_unvalidated_spotter"] = 1;  // TODO: remove? (field always == 1)
    if (smartActivationLog.Defined() && smartActivationLog->HasActivatedTimestamp() && smartActivationLog->HasActivatedDeviceId()) {
        TStringStream ss;
        ss << TimestampMilliSeconds(smartActivationLog->GetActivatedTimestamp()) << smartActivationLog->GetActivatedDeviceId();
        payload["multiactivation_id"] = ss.Str();
    } else {
        payload["multiactivation_id"] = "";// TODO: remove? (empty field)
    }

    TMessage message(TMessage::ToClient, std::move(jsonSV));
    NAliceProtocol::TWsEvent wsEvent;
    MessageToWsEvent(message, wsEvent);
    AppendDescription(wsEvent, eventDescriptionSuffix);
    return std::move(wsEvent);
}

NAliceProtocol::TWsEvent NAlice::NCuttlefish::NAppHostServices::BuildFallbackSpotterValidation(
    const TMaybe<NCachalotProtocol::TActivationLog>& smartActivationLog,
    const TMaybe<bool>& asrValid,
    const TString& refMessageId
) {
    // send validation ok
    TJsonValue jsonSV;
    auto& payload = BuildDirective(jsonSV, "Spotter", "Validation", refMessageId);
    payload["result"] = 1;
    int valid = 1;
    if (asrValid.Defined() && !asrValid.GetRef()) {
        valid = 0;
    }
    payload["valid"] = valid;
    payload["canceled_cause_of_multiactivation"] = 0;
    payload["allow_activation_by_unvalidated_spotter"] = 1;  // TODO: remove? (field always == 1)
    if (smartActivationLog.Defined() && smartActivationLog->HasActivatedTimestamp() && smartActivationLog->HasActivatedDeviceId()) {
        TStringStream ss;
        ss << TimestampMilliSeconds(smartActivationLog->GetActivatedTimestamp()) << smartActivationLog->GetActivatedDeviceId();
        payload["multiactivation_id"] = ss.Str();
    } else {
        payload["multiactivation_id"] = "";// TODO: remove? (empty field)
    }

    TMessage message(TMessage::ToClient, std::move(jsonSV));
    NAliceProtocol::TWsEvent wsEvent;
    static const TString result1 = " result=1";
    MessageToWsEvent(message, wsEvent);
    AppendDescription(wsEvent, result1);
    return std::move(wsEvent);
}

NAliceProtocol::TUniproxyDirective NAlice::NCuttlefish::NAppHostServices::BuildSessionLogMultiActivation(
    const NCachalotProtocol::TActivationLog& activationLog,
    const TString& refMessageId
) {
    NAliceProtocol::TUniproxyDirective directive;
    auto& directiveSessionLog = *directive.MutableSessionLog();
    directiveSessionLog.SetName("Directive");
    directiveSessionLog.SetAction("response");
    TJsonValue value;
    value["ForEvent"] = refMessageId;
    value["type"] = "MultiActivation";
    TJsonValue& body = value["Body"];
    if (activationLog.HasActivatedDeviceId()) {
        body["ActivatedDeviceId"] = activationLog.GetActivatedDeviceId();
    }
    if (activationLog.HasDeviceId()) {
        body["DeviceId"] = activationLog.GetDeviceId();
    }
    if (activationLog.HasYandexUid()) {
        body["YandexUid"] = activationLog.GetYandexUid();
    }
    if (activationLog.HasAvgRMS()) {
        body["AvgRMS"] = activationLog.GetAvgRMS();
    }
    if (activationLog.HasActivatedRMS()) {
        body["ActivatedRMS"] = activationLog.GetActivatedRMS();
    }
    if (activationLog.HasMultiActivationReason()) {
        body["MultiActivationReason"] = activationLog.GetMultiActivationReason();
    } else {
        body["MultiActivationReason"] = "OK";
    }
    if (activationLog.HasSpotterValidatedBy()) {
        body["SpotterValidatedBy"] = activationLog.GetSpotterValidatedBy();
    }
    if (activationLog.HasThisSpotterIsValid()) {
        body["ThisSpotterIsValid"] = activationLog.GetThisSpotterIsValid();
    }
    if (activationLog.HasFreshnessDeltaMilliSeconds()) {
        body["Timings"]["SecondWindowMilliseconds"] = activationLog.GetFreshnessDeltaMilliSeconds();
    } else {
        body["Timings"]["SecondWindowMilliseconds"] = 2500;
    }
    body["Timings"]["FirstWindowSeconds"] = 0;  // TODO:? remove ?
    if (activationLog.HasTimestamp()) {
        body["Timestamp"] = TimestampMilliSeconds(activationLog.GetTimestamp());
    }
    if (activationLog.HasActivatedTimestamp()) {
        body["ActivatedTimestamp"] = TimestampMilliSeconds(activationLog.GetActivatedTimestamp());
    }
    if (activationLog.HasFinishTimestamp()) {
        body["FinishTimestamp"] = TimestampMilliSeconds(activationLog.GetFinishTimestamp());
    }
    //TODO:?    "ActivatedTimestamp": None,
    //TODO:?    "AnotherLouderSpotter": None,
    //TODO:?    "Key": None,
    directiveSessionLog.SetValue(WriteJson(value, /*formatOutput=*/ false));
    return directive;
}
