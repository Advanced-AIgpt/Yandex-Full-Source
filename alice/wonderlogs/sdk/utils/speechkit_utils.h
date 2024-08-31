#pragma once

#include "alice/library/json/json.h"

#include <alice/library/client/protos/client_info.pb.h>
#include <alice/library/experiments/utils.h>
#include <alice/megamind/protos/analytics/megamind_analytics_info.pb.h>
#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/megamind/protos/common/environment_state.pb.h>
#include <alice/megamind/protos/common/events.pb.h>
#include <alice/megamind/protos/common/location.pb.h>
#include <alice/megamind/protos/speechkit/request.pb.h>
#include <alice/megamind/protos/speechkit/response.pb.h>

#include <library/cpp/json/json_value.h>

#include <util/generic/maybe.h>

namespace NAlice::NWonderlogs {

struct TVinsLikeRequest {
    explicit TVinsLikeRequest(const TSpeechKitRequestProto& skRequest);

    NJson::TJsonValue DumpJson() const;

    struct TEvent {
        struct TPayload {
            TString Result;
            TString ErrorText;
            google::protobuf::Struct Data;
        };

        TString InputSource;
        TMaybe<bool> EndOfUtterance;
        TMaybe<TString> Text;
        TMaybe<ui32> HypothesisNumber;
        TPayload Payload;
    };

    EEventType eventType;
    TMaybe<TString> DialogId;
    TString RequestId;
    TString PrevReqId;
    ui32 SequenceNumber = 0;
    TString Uuid;
    TString DeviceId;
    TString Lang;
    TString ClientTime;
    TMaybe<google::protobuf::Struct> CallbackArgs;
    TMaybe<TString> CallbackName;
    TEvent Utterance;
    TMaybe<TLocation> Location;
    TMaybe<bool> ResetSession;
    TMaybe<bool> VoiceSession;
    TMaybe<google::protobuf::Struct> LaasRegion;
    TMaybe<TExperimentsProto> Experiments;
    TVector<ui64> TestIDs;
    TClientInfoProto AppInfo;
    TDeviceState DeviceState;
    TSpeechKitRequestProto::TRequest::TAdditionalOptions AdditionalOptions;
    TMaybe<TVector<TEnvironmentDeviceInfo>> EnvironmentState;
    ui64 ServerTimeMs = 0;
};

struct TVinsLikeResponse {
    explicit TVinsLikeResponse(const TSpeechKitResponseProto& skResponse);
    explicit TVinsLikeResponse(const NJson::TJsonValue& skResponse);

    NJson::TJsonValue DumpJson() const;

    bool Empty = false;
    bool ForceVoiceAnswer = true;
    bool ShouldListen;
    TMaybe<TString> VoiceText;
    NJson::TJsonValue SpecialButtons = NJson::EJsonValueType::JSON_ARRAY;
    NJson::TJsonValue Suggests = NJson::EJsonValueType::JSON_ARRAY;
    TMaybe<NJson::TJsonValue> Templates;
    TMaybe<NJson::TJsonValue> QualityStorage;
    TMaybe<NJson::TJsonValue> Experiments;
    TMaybe<TString> DirectivesExecutionPolicy;
    TMaybe<NJson::TJsonValue> Cards;
    TMaybe<NJson::TJsonValue> Directives;
    TMaybe<bool> ContainsSensitiveData;
    TString ResponseId;
};

struct TVinsLikeLog {
    TVinsLikeLog(const TSpeechKitRequestProto& skRequest, const TSpeechKitResponseProto& skResponse);

    NJson::TJsonValue DumpJson() const;

    const TSpeechKitRequestProto& SpeechkitRequest;
    const TSpeechKitResponseProto& SpeechkitResponse;
};

} // namespace NAlice::NWonderlogs
