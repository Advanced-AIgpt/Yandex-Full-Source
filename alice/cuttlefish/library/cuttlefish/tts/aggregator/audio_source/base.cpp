#include "base.h"

#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>

#include <alice/cuttlefish/library/apphost/item_parser.h>

#include <apphost/lib/proto_answers/http.pb.h>

#include <google/protobuf/util/message_differencer.h>

namespace NAlice::NCuttlefish::NAppHostServices {

namespace {

bool IsEmptyTiming(const TTS::GenerateResponse::Timings& timing) {
    static TTS::GenerateResponse::Timings emptyTiming;
    return google::protobuf::util::MessageDifferencer::Equals(timing, emptyTiming);
}

class THttpResponseAudioSource : public IAudioSource {
public:
    THttpResponseAudioSource(
        const TLogContext& logContext,
        TSourceMetrics& metrics,
        const TString& itemType
    )
        : IAudioSource(logContext, metrics)
        , ItemType_(itemType)
    {
        if (ItemType_.StartsWith(ITEM_TYPE_PREFIX_S3_AUDIO_HTTP_RESPONSE)) {
            Type_ = IAudioSource::EType::S3_AUDIO;
        } else {
            Type_ = IAudioSource::EType::UNKNOWN_HTTP;
        }
    }

protected:
    void TryReceiveData(
        NAppHost::TServiceContextPtr ahContext,
        const THashMap<TString, NTts::TCacheGetResponse>& /* cacheGetResponses */,
        const THashMap<ui32, TVector<NAliceProtocol::TAudio>>& /* audioItems */
    ) override {
        const auto& itemRefs = ahContext->GetProtobufItemRefs(ItemType_, NAppHost::EContextItemSelection::Input);
        if (!itemRefs.empty()) {
            NAppHostHttp::THttpResponse rsp;

            try {
                ParseProtobufItem(*itemRefs.begin(), rsp);
            } catch (...) {
                Metrics_.PushRate("parse", "error", ToString(Type_));
                ythrow yexception() << "Failed to parse http response: " << CurrentExceptionMessage();
            }

            Metrics_.RateHttpCode(rsp.GetStatusCode(), ToString(Type_));

            if (rsp.GetStatusCode() != 200) {
                // Do not clear content if we have error
                LogContext_.LogEventErrorCombo<NEvClass::RecvFromAppHostHttpResponseItem>(rsp.ShortUtf8DebugString(), ItemType_);
                Metrics_.PushRate("response", "error", ToString(Type_));
                ythrow yexception() << "Bad http response: " << rsp.GetStatusCode();
            }

            {
                // Clear audio data/binary protobuf if response is ok
                NAppHostHttp::THttpResponse rspCopy = rsp;
                rspCopy.SetContent(TStringBuilder() << "Content size=" << rsp.GetContent().size());
                LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostHttpResponseItem>(rspCopy.ShortUtf8DebugString(), ItemType_);
            }

            try {
                ParseDataFromHttpResponse(rsp);
            } catch (...) {
                Metrics_.PushRate("parse", "error", ToString(Type_));
                ythrow yexception() << "Failed to parse http response: " << CurrentExceptionMessage();
            }

            UpdateMetrics(CurrentData_.Audio_.size(), CurrentData_.Timings_.size());
            State_ = IAudioSource::EState::STREAM_FINISHED;
        }
    }

private:
    void ParseDataFromHttpResponse(
        const NAppHostHttp::THttpResponse& response
    ) {
        CurrentData_.Audio_ = response.GetContent();

        // There are no timings and duration in http response
        CurrentData_.Timings_ = {};
        TotalDuration_ = TDuration::Zero();
    }

private:
    const TString ItemType_;
};

class TCacheGetResponseAudioSource : public IAudioSource {
public:
    TCacheGetResponseAudioSource(
        const TLogContext& logContext,
        TSourceMetrics& metrics,
        const TString& cacheKey
    )
        : IAudioSource(logContext, metrics)
        , CacheKey_(cacheKey)
    {
        Type_ = IAudioSource::EType::TTS_CACHE;
    }

protected:
    void TryReceiveData(
        NAppHost::TServiceContextPtr /* ahContext */,
        const THashMap<TString, NTts::TCacheGetResponse>& cacheGetResponses,
        const THashMap<ui32, TVector<NAliceProtocol::TAudio>>& /* audioItems */
    ) override {
        if (const auto ptr = cacheGetResponses.FindPtr(CacheKey_)) {
            switch (ptr->GetStatus()) {
                case NTts::ECacheGetResponseStatus::HIT: {
                    ProcessCacheGetResponse(*ptr);
                    UpdateMetrics(CurrentData_.Audio_.size(), CurrentData_.Timings_.size());
                    State_ = IAudioSource::EState::STREAM_FINISHED;
                    break;
                }
                case NTts::ECacheGetResponseStatus::MISS: {
                    State_ = IAudioSource::EState::NO_DATA_IN_RESPONSE;
                    break;
                }
                default: {
                    ythrow yexception() << ptr->GetErrorMessage();
                }
            }
        }
    }

private:
    void ProcessCacheGetResponse(
        const NTts::TCacheGetResponse& cacheGetResponse
    ) {
        CurrentData_.Audio_ = cacheGetResponse.GetCacheEntry().GetAudio();
        for (const auto& timing : cacheGetResponse.GetCacheEntry().GetTimings()) {
           if (IsEmptyTiming(timing)) {
                // https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/processors/tts.py?rev=r7978885#L501-502
                // Skip empty timings
                continue;
            }
            CurrentData_.Timings_.push_back(timing);
        }
        TotalDuration_ = TDuration::Seconds(cacheGetResponse.GetCacheEntry().GetDuration());
    }

private:
    const TString CacheKey_;
};

class TProtocolAudioAudioSource : public IAudioSource {
public:
    TProtocolAudioAudioSource(
        const TLogContext& logContext,
        TSourceMetrics& metrics,
        ui32 reqSeqNo
    )
        : IAudioSource(logContext, metrics)
        , ReqSeqNo_(reqSeqNo)
        , AudioItemsPtr_(0)
    {
        Type_ = IAudioSource::EType::TTS_BACKEND;
    }

protected:
    void TryReceiveData(
        NAppHost::TServiceContextPtr /* ahContext */,
        const THashMap<TString, NTts::TCacheGetResponse>& /* cacheGetResponses */,
        const THashMap<ui32, TVector<NAliceProtocol::TAudio>>& audioItems
    ) override {
        if (!audioItems.contains(ReqSeqNo_)) {
            return;
        }

        const auto& myAudio = audioItems.at(ReqSeqNo_);
        if (myAudio.empty()) {
            return;
        }

        ui64 oldAudioSize = CurrentData_.Audio_.size();
        ui64 oldTimingsCnt = CurrentData_.Timings_.size();
        for (ui32 itemPtr = AudioItemsPtr_; itemPtr < myAudio.size(); ++itemPtr) {
            const auto& currentAudio = myAudio[itemPtr];

            if (State_ != IAudioSource::EState::STREAM_FINISHED &&
                currentAudio.GetTtsBackendResponse().GetGenerateResponse().has_responsecode() &&
                currentAudio.GetTtsBackendResponse().GetGenerateResponse().responsecode() != 200
            ) {
                ythrow yexception()
                    << "Audio stream error with code: '"
                    << static_cast<ui32>(currentAudio.GetTtsBackendResponse().GetGenerateResponse().responsecode())
                    << "', message: '"
                    << currentAudio.GetTtsBackendResponse().GetGenerateResponse().message()
                    << "'"
                ;
            }

            switch (currentAudio.GetMessageCase()) {
                case NAliceProtocol::TAudio::MessageCase::kBeginStream: {
                    ProcessBeginStream(currentAudio.GetBeginStream());
                    // Process meta info after stream started
                    ProcessMetaInfo(currentAudio.GetTtsBackendResponse().GetGenerateResponse());
                    break;
                }
                case NAliceProtocol::TAudio::MessageCase::kChunk: {
                    ProcessAudioChunk(currentAudio.GetChunk());
                    ProcessMetaInfo(currentAudio.GetTtsBackendResponse().GetGenerateResponse());
                    break;
                }
                case NAliceProtocol::TAudio::MessageCase::kEndStream: {
                    // Process meta info before stream finished
                    ProcessMetaInfo(currentAudio.GetTtsBackendResponse().GetGenerateResponse());
                    ProcessEndStream(currentAudio.GetEndStream());
                    break;
                }
                case NAliceProtocol::TAudio::MessageCase::kMetaInfoOnly: {
                    // Just process meta info
                    ProcessMetaInfo(currentAudio.GetTtsBackendResponse().GetGenerateResponse());
                    break;
                }
                case NAliceProtocol::TAudio::MessageCase::kBeginSpotter:
                case NAliceProtocol::TAudio::MessageCase::kEndSpotter: {
                    Metrics_.PushRate("unexpectedaudiomessage", "error", ToString(Type_));
                    LogContext_.LogEventErrorCombo<NEvClass::ErrorMessage>(
                        TStringBuilder()
                            << "Unexpected audio message with case: '"
                            << NAliceProtocol::TAudio::descriptor()->FindFieldByNumber(currentAudio.GetMessageCase())->name()
                            << "'"
                    );
                    break;
                }
                case NAliceProtocol::TAudio::MessageCase::MESSAGE_NOT_SET: {
                    Metrics_.PushRate("audiomessagenotset", "error", ToString(Type_));
                    LogContext_.LogEventErrorCombo<NEvClass::ErrorMessage>("Audio message case not set");
                    break;
                }
            }
        }

        AudioItemsPtr_ = myAudio.size();
        UpdateMetrics(CurrentData_.Audio_.size() - oldAudioSize, CurrentData_.Timings_.size() - oldTimingsCnt);
    }

private:
    void ProcessBeginStream(const NAliceProtocol::TBeginStream& /* beginStream */) {
        switch (State_) {
            case IAudioSource::EState::STREAM_NOT_STARTED: {
                State_ = IAudioSource::EState::STREAM_STARTED;
                break;
            }
            case IAudioSource::EState::STREAM_STARTED: {
                Metrics_.PushRate("doublebeginstream", "error", ToString(Type_));
                LogContext_.LogEventErrorCombo<NEvClass::WarningMessage>("Stream started twice or more");
                break;
            }
            case IAudioSource::EState::STREAM_FINISHED: {
                Metrics_.PushRate("beginstreamafterendstream", "error", ToString(Type_));
                LogContext_.LogEventErrorCombo<NEvClass::WarningMessage>("Got BeginStream after EndStream");
                break;
            }
            case IAudioSource::EState::ERROR:
            case IAudioSource::EState::NO_DATA_IN_RESPONSE: {
                ythrow yexception() << "Got BeginStream in '" << ToString(State_) << "' state, this case must be unreachable";
            }
        }
    }

    void ProcessAudioChunk(const NAliceProtocol::TAudioChunk& audioChunk) {
        switch (State_) {
            case IAudioSource::EState::STREAM_NOT_STARTED: {
                ythrow yexception() << "Got AudioChunk before stream started";
            }
            case IAudioSource::EState::STREAM_STARTED: {
                CurrentData_.Audio_ += audioChunk.GetData();
                break;
            }
            case IAudioSource::EState::STREAM_FINISHED: {
                Metrics_.PushRate("audiochunkafterendstream", "error", ToString(Type_));
                LogContext_.LogEventErrorCombo<NEvClass::WarningMessage>("Got AudioChunk after stream finished");
                break;
            }
            case IAudioSource::EState::ERROR:
            case IAudioSource::EState::NO_DATA_IN_RESPONSE: {
                ythrow yexception() << "Got AudioChunk in '" << ToString(State_) << "' state, this case must be unreachable";
            }
        }
    }

    void ProcessEndStream(const NAliceProtocol::TEndStream& /* endStream */) {
        switch (State_) {
            case IAudioSource::EState::STREAM_NOT_STARTED: {
                ythrow yexception() << "Got EndStream before stream started";
            }
            case IAudioSource::EState::STREAM_STARTED: {
                State_ = IAudioSource::EState::STREAM_FINISHED;
                break;
            }
            case IAudioSource::EState::STREAM_FINISHED: {
                Metrics_.PushRate("doubleendstream", "error", ToString(Type_));
                LogContext_.LogEventErrorCombo<NEvClass::WarningMessage>("Got EndStream twice or more");
                break;
            }
            case IAudioSource::EState::ERROR:
            case IAudioSource::EState::NO_DATA_IN_RESPONSE: {
                ythrow yexception() << "Got EndStream in '" << ToString(State_) << "' state, this case must be unreachable";
            }
        }
    }

    void ProcessMetaInfo(const TTS::GenerateResponse& generateResponse) {
        switch (State_) {
            case IAudioSource::EState::STREAM_NOT_STARTED: {
                Metrics_.PushRate("metabeforebeginstream", "error", ToString(Type_));
                LogContext_.LogEventErrorCombo<NEvClass::WarningMessage>("Got meta info before stream started");
                break;
            }
            case IAudioSource::EState::STREAM_STARTED: {
                ProcessTimings(generateResponse);
                ProcessDuration(generateResponse);
                break;
            }
            case IAudioSource::EState::STREAM_FINISHED: {
                Metrics_.PushRate("metaafterendstream", "error", ToString(Type_));
                LogContext_.LogEventErrorCombo<NEvClass::WarningMessage>("Got meta info after stream finished");
                break;
            }
            case IAudioSource::EState::ERROR:
            case IAudioSource::EState::NO_DATA_IN_RESPONSE: {
                ythrow yexception() << "Got meta info in '" << ToString(State_) << "' state, this case must be unreachable";
            }
        }
    }

    void ProcessTimings(const TTS::GenerateResponse& generateResponse) {
        if (generateResponse.has_timings() && !IsEmptyTiming(generateResponse.timings())) {
            CurrentData_.Timings_.push_back(generateResponse.timings());
        }
    }

    void ProcessDuration(const TTS::GenerateResponse& generateResponse) {
        if (generateResponse.has_duration()) {
            TotalDuration_ += TDuration::Seconds(generateResponse.duration());
        }
    }

private:
    ui32 ReqSeqNo_;
    size_t AudioItemsPtr_;
};

} // namespace

TAudioSourcePtr CreateAudioSource(
    const NTts::TAggregatorRequest::TAudioSource& source,
    const TString& cacheKey,
    const TLogContext& logContext,
    TSourceMetrics& metrics
) {
    switch (source.GetSourceTypeCase()) {
        case NTts::TAggregatorRequest::TAudioSource::kHttpResponse: {
            return MakeHolder<THttpResponseAudioSource>(
                logContext,
                metrics,
                source.GetHttpResponse().GetItemType()
            );
        };
        case NTts::TAggregatorRequest::TAudioSource::kCacheGetResponse: {
            return MakeHolder<TCacheGetResponseAudioSource>(
                logContext,
                metrics,
                cacheKey
            );
        };
        case NTts::TAggregatorRequest::TAudioSource::kAudio: {
            return MakeHolder<TProtocolAudioAudioSource>(
                logContext,
                metrics,
                source.GetAudio().GetReqSeqNo()
            );
        };
        case NTts::TAggregatorRequest::TAudioSource::SOURCETYPE_NOT_SET: {
            ythrow yexception() << "Source type not set";
        };
    }
}

}  // namespace NAlice::NCuttlefish::NAppHostServices
