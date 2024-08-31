#include "tts.h"

#include <voicetech/library/proto_api/ttsbackend.pb.h>

#include <util/string/builder.h>

namespace NAlice::NCuttlefish {

namespace {

template<typename T>
void CensoreText(T& text) {
    text = TStringBuilder() << "<CENSORED>, size = " << text.size();
}

}  // namespace

void Censore(::NTts::TRequest& ttsRequest) {
    if (ttsRequest.GetDoNotLogTexts()) {
        if (ttsRequest.HasText() && !ttsRequest.GetText().empty()) {
            CensoreText(*ttsRequest.MutableText());
        }
    }
}

void Censore(::NTts::TBackendRequest& ttsBackendRequest) {
    if (ttsBackendRequest.GetDoNotLogTexts()) {
        if (ttsBackendRequest.HasGenerate() && ttsBackendRequest.GetGenerate().has_text() && !ttsBackendRequest.GetGenerate().text().empty()) {
            CensoreText(*ttsBackendRequest.MutableGenerate()->mutable_text());
        }
    }
}

void Censore(::NTts::TRequestSenderRequest& ttsRequestSenderRequest) {
    for (auto& audioPartGenerateRequest : *ttsRequestSenderRequest.MutableAudioPartGenerateRequests()) {
        // So we need to censore only backend requests

        if (audioPartGenerateRequest.GetRequest().GetRequestCase() == NTts::TRequestSenderRequest::TAudioPartGenerateRequest::TRequest::kBackendRequest) {
            if (ttsRequestSenderRequest.GetDoNotLogTexts()) {
                // Force censore
                audioPartGenerateRequest.MutableRequest()->MutableBackendRequest()->SetDoNotLogTexts(true);
            }
            Censore(*audioPartGenerateRequest.MutableRequest()->MutableBackendRequest());
        }
    }
}

void Censore(::NTts::TAggregatorRequest& ttsAggregatorRequest) {
    if (ttsAggregatorRequest.GetDoNotLogTexts()) {
        // Do nothing
        // There are no info about texts in aggregator request for now
    }
}

}
