#pragma once

#include <alice/cuttlefish/library/cuttlefish/common/metrics.h>

#include <alice/cuttlefish/library/logging/log_context.h>

#include <alice/cuttlefish/library/protos/audio.pb.h>
#include <alice/cuttlefish/library/protos/tts.pb.h>

#include <voicetech/library/proto_api/ttsbackend.pb.h>

#include <apphost/api/service/cpp/service.h>

namespace NAlice::NCuttlefish::NAppHostServices {

class IAudioSource {
public:
    enum EState {
        // No response for source received
        STREAM_NOT_STARTED = 0      /* "stream_not_started" */,

        // Any data received, but stream is not completed
        STREAM_STARTED = 1          /* "stream_started" */,

        // All data received
        // Stream ended correctly
        STREAM_FINISHED = 3         /* "stream_finished" */,

        // An error has occurred and data can no longer
        // be retrieved from this source
        ERROR = 4                   /* "error" */,

        // Some specific sources may answer correctly,
        // but their answer will not contain data,
        // for example cahce miss for cache sources
        NO_DATA_IN_RESPONSE = 5     /* "no_data_in_response" */,
    };

    enum EType {
        NOT_SET = 0       /* "notset" */,
        UNKNOWN_HTTP = 1  /* "unknownhttp" */,
        S3_AUDIO = 2      /* "s3audio" */,
        TTS_CACHE = 3     /* "ttscache" */,
        TTS_BACKEND = 4   /* "ttsbackend" */,
        MERGED = 5        /* "merged" */,
    };

    struct TData {
        TString Audio_;
        TVector<TTS::GenerateResponse::Timings> Timings_;
    };

public:
    IAudioSource(
        const TLogContext& logContext,
        TSourceMetrics& metrics
    )
        : State_(EState::STREAM_NOT_STARTED)
        , LogContext_(logContext)
        , Metrics_(metrics)
        , CurrentData_()
        , TotalDuration_(TDuration::Zero())
    {}
    virtual ~IAudioSource() = default;

    EState SafeTryReceiveData(
        NAppHost::TServiceContextPtr ahContext,

        // Yes, we can get them from apphost context
        // but sometimes we need to get same cache key several times for different sources,
        // so it's optimal extract them externally and pass to this method
        const THashMap<TString, NTts::TCacheGetResponse>& cacheGetResponses,

        // For same reasons as for cacheGetResponses it's more effectively to preprocess
        // all audio items externally
        const THashMap<ui32, TVector<NAliceProtocol::TAudio>>& audioItems
    ) {
        if (State_ != EState::STREAM_NOT_STARTED && State_ != EState::STREAM_STARTED) {
            return State_;
        }

        try {
            TryReceiveData(ahContext, cacheGetResponses, audioItems);
        } catch (...) {
            State_ = EState::ERROR;
            ErrorMessage_ = CurrentExceptionMessage();
        }

        return State_;
    }

    EState GetState() const {
        return State_;
    }

    EType GetType() const {
        return Type_;
    }

    const TString& GetErrorMessage() const {
        return ErrorMessage_;
    }

    bool HasNewDataToSend() const {
        return
            (State_ == EState::STREAM_STARTED || State_ == EState::STREAM_FINISHED) &&
            (!CurrentData_.Audio_.empty() || !CurrentData_.Timings_.empty())
        ;
    }

    // WARNING: Clears the buffer
    // Next calls before TryReceiveData will return empty TData
    TData GetNewDataToSend() {
        TData data;
        std::swap(data, CurrentData_);
        return std::move(data);
    }

    // WARNING: Total duration is correct only in STREAM_FINISHED state
    TDuration GetTotalDuration() const {
        return TotalDuration_;
    }

    static NTts::TAggregatorAudioMetaInfo::EAudioSource TypeToAggregatorAudioMetaInfoEnum(EType type) {
        switch (type) {
            case EType::NOT_SET: {
                return NTts::TAggregatorAudioMetaInfo::NOT_SET;
            }
            case EType::UNKNOWN_HTTP: {
                return NTts::TAggregatorAudioMetaInfo::NOT_SET;
            }
            case EType::S3_AUDIO: {
                return NTts::TAggregatorAudioMetaInfo::S3_AUDIO;
            }
            case EType::TTS_CACHE: {
                return NTts::TAggregatorAudioMetaInfo::TTS_CACHE;
            }
            case EType::TTS_BACKEND: {
                return NTts::TAggregatorAudioMetaInfo::TTS_BACKEND;
            }
            case EType::MERGED: {
                return NTts::TAggregatorAudioMetaInfo::MERGED;
            }
        }

        Y_UNREACHABLE();
    }

protected:
    // WARNING: This function must be called only from SafeTryReceiveData
    // It's guaranteed that state is STREAM_NOT_STARTED or STREAM_STARTED when this function called
    virtual void TryReceiveData(
        NAppHost::TServiceContextPtr ahContext,
        const THashMap<TString, NTts::TCacheGetResponse>& cacheGetResponses,
        const THashMap<ui32, TVector<NAliceProtocol::TAudio>>& audioItems
    ) = 0;

    void UpdateMetrics(ui64 audioSize, ui64 timingsCnt) {
        Metrics_.PushRate(audioSize, "received", "ok", ToString(Type_));
        Metrics_.PushRate(timingsCnt, "timings", "ok", ToString(Type_));
    }

protected:
    EState State_;
    TString ErrorMessage_;
    EType Type_;

    TLogContext LogContext_;

    // WARNING: Intended reference here
    // You must keep metrics alive for the rest of the object life
    TSourceMetrics& Metrics_;

    TData CurrentData_;

    // Sum of the durations from responses meta info
    // If some of responses has no meta info, duration of this response
    // assumed to equal zero
    // Need only to calculate TTS.Timings in the same way that it's done
    // in python uniproxy
    TDuration TotalDuration_;
};

using TAudioSourcePtr = THolder<IAudioSource>;

TAudioSourcePtr CreateAudioSource(
    const NTts::TAggregatorRequest::TAudioSource& source,
    const TString& cacheKey,
    const TLogContext& logContext,
    TSourceMetrics& metrics
);

}  // namespace NAlice::NCuttlefish::NAppHostServices
