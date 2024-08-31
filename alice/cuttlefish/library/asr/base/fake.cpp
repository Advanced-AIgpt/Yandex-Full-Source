#include "fake.h"

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/str.h>

using namespace NAlice::NAsr;

namespace {
    TVector<TString> defaultAsrResult{"эмулятор", "asr", "клиента"};
}

bool TFake::ProcessAsrRequest(const NProtobuf::TRequest& request) {
    if (Closed_) {
        return false;
    }

    if (request.HasInitRequest()) {
        return ProcessInitRequest(request.GetInitRequest());
    } else if (request.HasAddData()) {
        return ProcessAddData(request.GetAddData());
    } else if (request.HasEndOfStream()) {
        ProcessEndOfStream();
        return false;
    } else if (request.HasCloseConnection()) {
        ProcessCloseConnection();
        return false;
    }
    return true;
}

void TFake::CauseError(const TString& error) {
    if (Closed_) {
        return;
    }

    if (SendedInitResponse_) {
        NProtobuf::TResponse response;
        auto& addDataResponse = *response.MutableAddDataResponse();
        NProtobuf::FillRequiredDefaults(addDataResponse);
        addDataResponse.SetIsOk(false);
        addDataResponse.SetErrMsg(error);
        Callbacks_->OnAddDataResponse(response);
    } else {
        NProtobuf::TResponse response;
        auto& initResponse = *response.MutableInitResponse();
        NProtobuf::FillRequiredDefaults(initResponse);
        initResponse.SetIsOk(false);
        initResponse.SetErrMsg(error);
        Callbacks_->OnInitResponse(response);
    }
    Callbacks_->OnClosed();
    Closed_ = true;
}

bool TFake::ProcessInitRequest(const NProtobuf::TInitRequest& initRequest) {
    (void)initRequest;
    NProtobuf::TResponse response;
    auto& initResponse = *response.MutableInitResponse();
    SpotterValidation_ = initRequest.GetHasSpotterPart();
    NProtobuf::FillRequiredDefaults(initResponse);
    initResponse.SetIsOk(true);
    initResponse.SetTopic("test_topic2");
    Callbacks_->OnInitResponse(response);
    SendedInitResponse_ = true;
    return true;
}

bool TFake::ProcessAddData(const NProtobuf::TAddData& addData) {
    if (Closed_) {
        return false;
    }

    if (addData.HasAudioData()) {
        RecvBytes_ += addData.GetAudioData().size();
    }
    if (SpotterValidation_) {
        if (!addData.GetSpotterChunk() && RecvBytes_ >= 3000) {
            SpotterValidation_ = false;
            Callbacks_->OnSpotterValidation(true);
        }
    }
    NProtobuf::TResponse response;
    FillAddDataResponse(defaultAsrResult, *response.MutableAddDataResponse());
    Callbacks_->OnAddDataResponse(response);
    return true;
}

void TFake::ProcessEndOfStream() {
    if (Closed_) {
        return;
    }

    if (SpotterValidation_) {
        SpotterValidation_ = false;
        Callbacks_->OnSpotterValidation(true);
    }

    NProtobuf::TResponse response;
    auto addDataResponse = response.MutableAddDataResponse();
    FillAddDataResponse(defaultAsrResult, *addDataResponse, 0);
    addDataResponse->SetResponseStatus(NProtobuf::EndOfUtterance);
    Callbacks_->OnAddDataResponse(response);
    Callbacks_->OnClosed();
    Closed_ = true;
}

void TFake::ProcessCloseConnection() {
    if (Closed_) {
        return;
    }

    Callbacks_->OnClosed();
    Closed_ = true;
}

void TFake::FillAddDataResponse(const TVector<TString>& words, NProtobuf::TAddDataResponse& addDataResponse, size_t messages) {
    NProtobuf::FillRequiredDefaults(addDataResponse);
    addDataResponse.SetIsOk(true);
    addDataResponse.SetMessagesCount(messages);
    DurationProcessedAudio_ += 100;
    addDataResponse.SetDurationProcessedAudio(DurationProcessedAudio_);
    auto hypo = addDataResponse.MutableRecognition()->AddHypos();
    NProtobuf::FillRequiredDefaults(*hypo);
    TString normalized;
    TStringOutput normStream(normalized);
    size_t useWords = Min(RecvBytes_/2000, words.size());
    for (size_t i = 0; i < useWords; ++i) {
        if (normalized.size()) {
            normStream << ' ';
        }
        normStream << words[i];
        hypo->AddWords(words[i]);
    }
    if (normalized) {
        hypo->SetNormalized(normalized);
    }
}
