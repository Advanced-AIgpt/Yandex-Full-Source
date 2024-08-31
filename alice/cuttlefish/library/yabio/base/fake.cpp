#include "fake.h"

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/str.h>

#include <algorithm>
#include <map>

using namespace NAlice::NYabio;

namespace {
    TVector<TString> defaultYabioResults{"yabio fake result1", "yabio fake result 2", "yabio fake result 3"};
}

void TFake::CauseError(NProtobuf::EResponseCode responseCode, const TString& error) {
    (void)error;  // current yabio protocol not has field for error text
    if (Closed_) {
        return;
    }

    if (SendedInitResponse_) {
        NProtobuf::TResponse response;
        auto addDataResponse = response.MutableAddDataResponse();
        addDataResponse->SetresponseCode(responseCode);
        Callbacks_->OnAddDataResponse(response);
    } else {
        NProtobuf::TResponse response;
        auto& initResponse = *response.MutableInitResponse();
        NProtobuf::FillRequiredDefaults(initResponse);
        initResponse.SetresponseCode(responseCode);
        Callbacks_->OnInitResponse(response);
    }
    Callbacks_->OnClosed();
    Closed_ = true;
}

bool TFake::ProcessInitRequest(NProtobuf::TInitRequest& initRequest) {
    if (initRequest.method() == NProtobuf::METHOD_CLASSIFY) {
        Classify_ = true;
        for (auto& ct : initRequest.classification_tags()) {
            ClassificationTags_.push_back(ct);
        }
    } else if (initRequest.method() == NProtobuf::METHOD_SCORE) {
        Classify_ = false;
        if (initRequest.has_context()) {
            Context_ = initRequest.Getcontext();
        } else {
            Context_.set_group_id(initRequest.group_id());
        }
    } else {
        NProtobuf::TResponse response;
        auto& initResponse = *response.MutableInitResponse();
        NProtobuf::FillRequiredDefaults(initResponse);
        initResponse.SetresponseCode(NProtobuf::RESPONSE_CODE_BAD_REQUEST);
        Callbacks_->OnInitResponse(response);
        SendedInitResponse_ = true;
        return false;
    }

    NProtobuf::TResponse response;
    auto& initResponse = *response.MutableInitResponse();
    NProtobuf::FillRequiredDefaults(initResponse);
    initResponse.SetresponseCode(NProtobuf::RESPONSE_CODE_OK);
    Callbacks_->OnInitResponse(response);
    SendedInitResponse_ = true;
    return true;
}

bool TFake::ProcessAddData(NProtobuf::TAddData& addData) {
    if (Closed_) {
        return false;
    }

    if (addData.HasaudioData()) {
        RecvBytes_ += addData.GetaudioData().size();
    }
    NProtobuf::TResponse response;
    FillAddDataResponse(defaultYabioResults, *response.MutableAddDataResponse());
    Callbacks_->OnAddDataResponse(response);
    if (addData.GetlastChunk()) {
        Callbacks_->OnClosed();
        Closed_ = true;
        return false;
    }

    return true;
}

void TFake::FillAddDataResponse(const TVector<TString>& words, NProtobuf::TAddDataResponse& addDataResponse, size_t messages) {
    (void)words;
    NProtobuf::FillRequiredDefaults(addDataResponse);
    addDataResponse.SetresponseCode(NProtobuf::RESPONSE_CODE_OK);
    addDataResponse.SetmessagesCount(messages);
    DurationProcessedAudio_ += 100;
    if (Classify_) {
        static const std::map<TString, TString> tagToClassName = {{"children", "adult"}, {"gender", "male"}};
        double confidence = DurationProcessedAudio_/1000.;
        if (confidence > 0.9) {
            confidence = 0.98;
        }

        for (const auto& [tag, className] : tagToClassName) {
            if (std::find(ClassificationTags_.begin(), ClassificationTags_.end(), tag) == ClassificationTags_.end()) {
                continue;
            }

            auto& simpleClassificationResults = *addDataResponse.add_classificationresults();
            simpleClassificationResults.set_tag(tag);
            simpleClassificationResults.set_classname(className);

            auto& bioResult = *addDataResponse.add_classification();
            bioResult.set_tag(tag);
            bioResult.set_classname(className);
            bioResult.set_confidence(confidence);
        }
    } else {
        if (!ContextHasNewEnrolling_ && RecvBytes_ > 2000) {
            NProtobuf::TVoiceprint voiceprint;
            voiceprint.set_compatibility_tag("fake_compatibility_tag");
            voiceprint.set_format("fake_format");
            voiceprint.set_source("fake_source");
            voiceprint.add_voiceprint(0.1);
            voiceprint.add_voiceprint(0.2);
            voiceprint.add_voiceprint(0.3);
            if (words) {
                // join works whith separator=' '
                TString text;
                TStringOutput so(text);
                for (auto& w : words) {
                    if (text.size()) {
                        so << ' ';
                    }
                    so << w;
                }
                voiceprint.set_text(text);
            }
            voiceprint.set_device_model("fake_device_model");
            voiceprint.set_device_id("fake_device_id");
            voiceprint.set_device_manufacturer("fake_device_manufacturer");
            *Context_.add_enrolling() = std::move(voiceprint);
            ContextHasNewEnrolling_ = true;
        }
        *addDataResponse.mutable_context() = Context_;
    }
}
