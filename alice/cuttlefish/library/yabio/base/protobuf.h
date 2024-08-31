#pragma once

#include <alice/cuttlefish/library/protos/yabio.pb.h>
#include <voicetech/library/proto_api/yabio.pb.h>

// create aliases inside arcadia style _N_amespace
namespace NAlice::NYabio::NProtobuf {
    using TRequest = ::NYabio::TRequest;
    using TResponse = ::NYabio::TResponse;
    using TInitRequest = YabioProtobuf::YabioRequest;
    using TInitResponse = YabioProtobuf::YabioResponse;
    using TAddData = YabioProtobuf::AddData;
    using TAddDataResponse = YabioProtobuf::AddDataResponse;
    using EResponseCode = YaldiProtobuf::ResponseCode;
    using TContext = YabioProtobuf::YabioContext;
    using TVoiceprint = YabioProtobuf::YabioVoiceprint;
    static const auto RESPONSE_CODE_OK = YaldiProtobuf::OK;
    static const auto RESPONSE_CODE_BAD_REQUEST = YaldiProtobuf::BadMessageFormatting;
    static const auto RESPONSE_CODE_INTERNAL_ERROR = YaldiProtobuf::InternalError;
    using EMethod = YabioProtobuf::Method;
    static const auto METHOD_SCORE = YabioProtobuf::Score;
    static const auto METHOD_CLASSIFY = YabioProtobuf::Classify;

    void FillRequiredDefaults(TInitRequest&);
    void FillRequiredDefaults(TInitResponse&);
    void FillRequiredDefaults(TAddDataResponse&);
}
