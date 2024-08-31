#pragma once

#include <alice/cuttlefish/library/protos/asr.pb.h>
#include <voicetech/asr/engine/proto_api/request.pb.h>
#include <voicetech/asr/engine/proto_api/response.pb.h>

// create aliases inside arcadia style _N_amespace
namespace NAlice::NAsr::NProtobuf {
    using TRequest = AsrEngineRequestProtobuf::TASRRequest;
    using TResponse = AsrEngineResponseProtobuf::TASRResponse;
    using TInitRequest = AsrEngineRequestProtobuf::TInitRequest;
    using TInitResponse = AsrEngineResponseProtobuf::TInitResponse;
    using TAddData = AsrEngineRequestProtobuf::TAddData;
    using TAddDataResponse = AsrEngineResponseProtobuf::TAddDataResponse;
    using TAsrHypo = AsrEngineResponseProtobuf::TAsrHypo;
    using TEndOfStream = AsrEngineRequestProtobuf::TEndOfStream;
    using TCloseConnection = AsrEngineRequestProtobuf::TCloseConnection;
    using EResponseStatus = AsrEngineResponseProtobuf::EResponseStatus;
    const EResponseStatus Active = AsrEngineResponseProtobuf::Active;
    const EResponseStatus EndOfUtterance = AsrEngineResponseProtobuf::EndOfUtterance;
    const EResponseStatus ValidationFailed = AsrEngineResponseProtobuf::ValidationFailed;
    using EEndOfUttMode = AsrEngineRequestProtobuf::EEndOfUttMode;
    const EEndOfUttMode NoEOU = AsrEngineRequestProtobuf::NoEOU;
    const EEndOfUttMode SingleUtterance = AsrEngineRequestProtobuf::SingleUtterance;
    const EEndOfUttMode MultiUtterance = AsrEngineRequestProtobuf::MultiUtterance;
    using TRecognitionOptions = AsrEngineRequestProtobuf::TRecognitionOptions;
    using EDegradationMode = AsrEngineRequestProtobuf::EDegradationMode;
    const EDegradationMode DegradationModeDisable = AsrEngineRequestProtobuf::Disable;
    const EDegradationMode DegradationModeAuto = AsrEngineRequestProtobuf::Auto;
    const EDegradationMode DegradationModeEnable = AsrEngineRequestProtobuf::Enable;

    void FillRequiredDefaults(TInitResponse&);
    void FillRequiredDefaults(TAddDataResponse&);
    void FillRequiredDefaults(TAsrHypo&);
}
