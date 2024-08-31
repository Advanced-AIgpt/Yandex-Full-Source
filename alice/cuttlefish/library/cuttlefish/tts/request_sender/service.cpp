#include "service.h"

#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/cuttlefish/stream_servant_base/base.h>

#include <alice/cuttlefish/library/apphost/item_parser.h>
#include <alice/cuttlefish/library/proto_censor/tts.h>
#include <alice/cuttlefish/library/protos/tts.pb.h>

#include <apphost/lib/proto_answers/http.pb.h>

namespace NAlice::NCuttlefish::NAppHostServices {

namespace {

// Parse cachalot cache miss responses and generate requests for tts backend
class TTtsRequestSender : public TStreamServantBase {
private:
    struct TAudioPartGenerateRequestInfo {
        NTts::TRequestSenderRequest::TAudioPartGenerateRequest Request_;
        // SequenceNumber in original order
        ui32 SequenceNumber_;
    };

    enum ESendConditionState {
        NOT_DEFINED = 0,
        SEND = 1,
        SKIP = 2,
    };

public:
    TTtsRequestSender(
        NAppHost::TServiceContextPtr ctx,
        TLogContext logContext
    )
        : TStreamServantBase(
            ctx,
            logContext,
            TTtsRequestSender::SOURCE_NAME
        )
        , WasCacheGetResponseStatusParseError_(false)
    {}

protected:
    bool ProcessFirstChunk() override {
        const auto requestSenderRequestItemRefs = AhContext_->GetProtobufItemRefs(ITEM_TYPE_TTS_REQUEST_SENDER_REQUEST, NAppHost::EContextItemSelection::Input);
        if (requestSenderRequestItemRefs.size() >= 1u) {
            try {
                ParseProtobufItem(*requestSenderRequestItemRefs.begin(), RequestSenderRequest_);
            } catch (...) {
                Metrics_.SetError("badrequest");
                OnError(TStringBuilder() << "Failed to parse request sender request: " << CurrentExceptionMessage(), /* isCritical = */ true);
                return false;
            }
        } else {
            Metrics_.SetError("norequest");
            OnError("Request sender request not found in first chunk", /* isCritical = */ true);
            return false;
        }

        LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostTtsRequestSenderRequest>(GetCensoredTtsRequestStr(RequestSenderRequest_));

        try {
            ProcessRequestSenderRequest();
        } catch (...) {
            Metrics_.SetError("badrequest");
            OnError(TStringBuilder() << "Failed to process no cache splitter request: " << CurrentExceptionMessage(), /* isCritical = */ true);
            return false;
        }

        return true;
    }

    bool ProcessInput() override {
        try {
            ProcessCacheGetResponseStatuses();
            RecalcConditionsAndProcessRequests();
        } catch (...) {
            Metrics_.SetError("badchunk");
            OnError(TStringBuilder() << "Failed to process new chunk: " << CurrentExceptionMessage(), /* isCritical = */ true);
            return false;
        }

        if (AllAudioPartGenerateRequestsAreProcessed()) {
            LogContext_.LogEventInfoCombo<NEvClass::InfoMessage>("All audio part generate requests are processed");
        }

        return true;
    }

    bool IsCompleted() override {
        return AllAudioPartGenerateRequestsAreProcessed();
    }

    TString GetErrorForIncompleteInput() override {
        return TStringBuilder()
           << "Got end of stream before all audio part generate requests send condition finalized. Processed "
           << (RequestSenderRequest_.GetAudioPartGenerateRequests().size() - AudioPartGenerateRequests_.size())
           << "/" << RequestSenderRequest_.GetAudioPartGenerateRequests().size()
        ;
    }

private:
    void ProcessRequestSenderRequest() {
        AudioPartGenerateRequests_.reserve(RequestSenderRequest_.GetAudioPartGenerateRequests().size());
        for (i32 requestId = 0; requestId < RequestSenderRequest_.GetAudioPartGenerateRequests().size(); ++requestId) {
            const auto& audioPartGenerateRequest = RequestSenderRequest_.GetAudioPartGenerateRequests()[requestId];

            if (audioPartGenerateRequest.GetSendCondition().GetConditionCase() == NTts::TRequestSenderRequest::TAudioPartGenerateRequest::TSendCondition::CONDITION_NOT_SET) {
                ythrow yexception() << "Send condition not set for '" << requestId << "' audio part generate request";
            }
            if (audioPartGenerateRequest.GetRequest().GetRequestCase() == NTts::TRequestSenderRequest::TAudioPartGenerateRequest::TRequest::REQUEST_NOT_SET) {
                ythrow yexception() << "Request not set for '" << requestId << "' audio part generate request";
            }

            AudioPartGenerateRequests_.push_back({
                .Request_ = audioPartGenerateRequest,
                .SequenceNumber_ = static_cast<ui32>(requestId),
            });
        }
    }

    void ProcessCacheGetResponseStatuses() {
        const auto ttsCacheGetResponseStatusItemRefs = AhContext_->GetProtobufItemRefs(ITEM_TYPE_TTS_CACHE_GET_RESPONSE_STATUS, NAppHost::EContextItemSelection::Input);

        for (const auto& ttsCacheGetResponseStatusItemRef : ttsCacheGetResponseStatusItemRefs) {
            NTts::TCacheGetResponseStatus cacheGetResponseStatus;
            try {
                ParseProtobufItem(ttsCacheGetResponseStatusItemRef, cacheGetResponseStatus);
                Metrics_.PushRate("cachegetresponsestatus", NTts::ECacheGetResponseStatus_Name(cacheGetResponseStatus.GetStatus()), "ttscache");
                LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostTtsCacheGetResponseStatus>(cacheGetResponseStatus.ShortUtf8DebugString());
                CacheGetResponseStatuses_[cacheGetResponseStatus.GetKey()] = std::move(cacheGetResponseStatus);
            } catch (...) {
                Metrics_.PushRate("parse", "error", "ttscache");
                LogContext_.LogEventErrorCombo<NEvClass::ErrorMessage>("Failed to parse tts cache get response status");
                WasCacheGetResponseStatusParseError_ = true;
            }
        }
    }

    void RecalcConditionsAndProcessRequests() {
        bool needFlush = false;
        TVector<TAudioPartGenerateRequestInfo> notDefined;
        for (auto& audioPartGenerateRequestInfo : AudioPartGenerateRequests_) {
            switch (GetSendConditionState(audioPartGenerateRequestInfo.Request_.GetSendCondition(), audioPartGenerateRequestInfo.SequenceNumber_)) {
                case NOT_DEFINED: {
                    notDefined.push_back(std::move(audioPartGenerateRequestInfo));
                    break;
                }
                case SEND: {
                    Metrics_.PushRate("request", "sent", GetMetricsSourceType(audioPartGenerateRequestInfo.Request_.GetRequest()));
                    LogContext_.LogEventInfoCombo<NEvClass::InfoMessage>(TStringBuilder() << "Send '" << audioPartGenerateRequestInfo.SequenceNumber_ << "' audio generate request");
                    SendRequest(audioPartGenerateRequestInfo.Request_.GetRequest(), audioPartGenerateRequestInfo.SequenceNumber_);
                    needFlush = true;
                    break;
                }
                case SKIP: {
                    Metrics_.PushRate("request", "skipped", GetMetricsSourceType(audioPartGenerateRequestInfo.Request_.GetRequest()));
                    LogContext_.LogEventInfoCombo<NEvClass::InfoMessage>(TStringBuilder() << "Skip '" << audioPartGenerateRequestInfo.SequenceNumber_ << "' audio generate request");
                    break;
                }
            }
        }

        if (needFlush) {
            AhContext_->IntermediateFlush();
        }
        AudioPartGenerateRequests_.swap(notDefined);
    }

    ESendConditionState GetSendConditionState(
        const NTts::TRequestSenderRequest::TAudioPartGenerateRequest::TSendCondition& sendCondition,
        ui32 sequenceNumber
    ) {
        switch (sendCondition.GetConditionCase()) {
            case NTts::TRequestSenderRequest::TAudioPartGenerateRequest::TSendCondition::kAlwaysSend: {
                return ESendConditionState::SEND;
            }
            case NTts::TRequestSenderRequest::TAudioPartGenerateRequest::TSendCondition::kCacheMissOrError: {
                return GetCacheMissOrErrorConditionState(sendCondition.GetCacheMissOrError());
            }
            case NTts::TRequestSenderRequest::TAudioPartGenerateRequest::TSendCondition::CONDITION_NOT_SET: {
                // Impossible situation (it was checked in ProcessRequestSenderRequest)
                // Just sanity check
                ythrow yexception() << "Send condition not set for '" << sequenceNumber << "' audio part generate request";
            }
        }
    }

    ESendConditionState GetCacheMissOrErrorConditionState(
        const NTts::TRequestSenderRequest::TAudioPartGenerateRequest::TSendCondition::TCacheMissOrError& cacheMissOrError
    ) {
        if (auto cacheGetResponseStatus = CacheGetResponseStatuses_.FindPtr(cacheMissOrError.GetKey())) {
            switch (cacheGetResponseStatus->GetStatus()) {
                case NTts::ECacheGetResponseStatus::ERROR:
                case NTts::ECacheGetResponseStatus::MISS: {
                    return ESendConditionState::SEND;
                }
                default: {
                    return ESendConditionState::SKIP;
                }
            }
        } else if (WasCacheGetResponseStatusParseError_) {
            // Error occurred but we can't match this error to audio part
            // so we just send all not defined items
            return ESendConditionState::SEND;
        } else {
            return ESendConditionState::NOT_DEFINED;
        }
    }

    void SendRequest(
        const NTts::TRequestSenderRequest::TAudioPartGenerateRequest::TRequest& request,
        ui32 sequenceNumber
    ) {
        switch (request.GetRequestCase()) {
            case NTts::TRequestSenderRequest::TAudioPartGenerateRequest::TRequest::kHttpRequest: {
                SendHttpRequest(request.GetItemType(), request.GetHttpRequest());
                break;
            }
            case NTts::TRequestSenderRequest::TAudioPartGenerateRequest::TRequest::kBackendRequest: {
                SendTtsBackendRequest(request.GetItemType(), request.GetBackendRequest());
                break;
            }
            case NTts::TRequestSenderRequest::TAudioPartGenerateRequest::TRequest::REQUEST_NOT_SET: {
                // Impossible situation (it was checked in ProcessRequestSenderRequest)
                // Just sanity check
                ythrow yexception() << "Request not set for '" << sequenceNumber << "' audio part generate request";
            }
        }
    }

    void SendHttpRequest(
        const TString& itemType,
        const NAppHostHttp::THttpRequest& httpRequest
    ) {
        AhContext_->AddProtobufItem(httpRequest, itemType);
        // Now there are only s3 audio requests here
        // So no censor here
        LogContext_.LogEventInfoCombo<NEvClass::SendToAppHostHttpRequestItem>(httpRequest.ShortUtf8DebugString(), itemType);
    }

    void SendTtsBackendRequest(
        const TString& itemType,
        const NTts::TBackendRequest& backendRequest
    ) {
        AhContext_->AddProtobufItem(backendRequest, itemType);
        LogContext_.LogEventInfoCombo<NEvClass::SendToAppHostTtsBackendRequest>(GetCensoredTtsRequestStr(backendRequest), itemType);
    }

    TStringBuf GetMetricsSourceType(const NTts::TRequestSenderRequest::TAudioPartGenerateRequest::TRequest& request) {
        switch (request.GetRequestCase()) {
            case NTts::TRequestSenderRequest::TAudioPartGenerateRequest::TRequest::kHttpRequest: {
                if (request.GetItemType().StartsWith(ITEM_TYPE_PREFIX_S3_AUDIO_HTTP_REQUEST)) {
                    return "s3audio";
                } else {
                    return "unknownhttp";
                }
            }
            case NTts::TRequestSenderRequest::TAudioPartGenerateRequest::TRequest::kBackendRequest: {
                return "ttsbackend";
            }
            case NTts::TRequestSenderRequest::TAudioPartGenerateRequest::TRequest::REQUEST_NOT_SET: {
                return "unknown";
            }
        }
    }

    bool AllAudioPartGenerateRequestsAreProcessed() {
        return AudioPartGenerateRequests_.empty();
    }

private:
    static constexpr TStringBuf SOURCE_NAME = "tts_request_sender";

    NTts::TRequestSenderRequest RequestSenderRequest_;
    TVector<TAudioPartGenerateRequestInfo> AudioPartGenerateRequests_;

    THashMap<TString, NTts::TCacheGetResponseStatus> CacheGetResponseStatuses_;
    bool WasCacheGetResponseStatusParseError_ = false;
};

} // namespace

NThreading::TPromise<void> TtsRequestSender(NAppHost::TServiceContextPtr ctx, TLogContext logContext) {
    TIntrusivePtr<TTtsRequestSender> stream(new TTtsRequestSender(ctx, std::move(logContext)));
    stream->OnNextInput();
    return stream->GetFinishPromise();
}

}  // namespace NAlice::NCuttlefish::NAppHostServices
