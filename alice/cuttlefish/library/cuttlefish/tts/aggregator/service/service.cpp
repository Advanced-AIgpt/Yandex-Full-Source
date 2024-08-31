#include "service.h"

#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>

#include <alice/cuttlefish/library/cuttlefish/stream_servant_base/base.h>
#include <alice/cuttlefish/library/cuttlefish/tts/aggregator/audio_source/base.h>
#include <alice/cuttlefish/library/cuttlefish/tts/aggregator/output_audio_stream/base.h>

#include <alice/cuttlefish/library/apphost/item_parser.h>
#include <alice/cuttlefish/library/proto_censor/tts.h>

#include <alice/cuttlefish/library/protos/audio.pb.h>
#include <alice/cuttlefish/library/protos/tts.pb.h>

namespace NAlice::NCuttlefish::NAppHostServices {

namespace {

// Parse tts splitter, tts backends, cachalot tts cache, s3 audio responses and generate audio
class TTtsAggregator : public TStreamServantBase {
private:
    struct TSource {
        TAudioSourcePtr Source_;
        ui32 SequenceNumber_;
    };

    struct TMultiSource {
        TVector<TSource> Sources_;
        ui32 SequenceNumber_;
    };

public:
    TTtsAggregator(
        NAppHost::TServiceContextPtr ctx,
        TLogContext logContext
    )
        : TStreamServantBase(
            ctx,
            logContext,
            TTtsAggregator::SOURCE_NAME
        )
        , CurrentDuration_(TDuration::Zero())
        , FirstChunkStatsSent_(false)
    {}

protected:
    bool ProcessFirstChunk() override {
        const auto aggregatorRequestItemRefs = AhContext_->GetProtobufItemRefs(ITEM_TYPE_TTS_AGGREGATOR_REQUEST, NAppHost::EContextItemSelection::Input);
        if (!aggregatorRequestItemRefs.empty()) {
            try {
                ParseProtobufItem(*aggregatorRequestItemRefs.begin(), AggregatorRequest_);
            } catch (...) {
                Metrics_.SetError("badrequest");
                OnError(TStringBuilder() << "Failed to parse aggregator request: " << CurrentExceptionMessage(), /* isCritical = */ true);
                return false;
            }
        } else {
            Metrics_.SetError("norequest");
            OnError("Aggregator request not found in first chunk", /* isCritical = */ true);
            return false;
        }

        LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostTtsAggregatorRequest>(GetCensoredTtsRequestStr(AggregatorRequest_));

        try {
            ProcessAggregatorRequest();
        } catch (...) {
            Metrics_.SetError("badrequest");
            OnError(TStringBuilder() << "Failed to process aggregator request: " << CurrentExceptionMessage(), /* isCritical = */ true);
            return false;
        }

        return true;
    }

    bool ProcessInput() override {
        try {
            // WARNING: order is important

            // Preprocess some item types
            // Optimization to save cpu and memory
            GetNewCacheGetResponses();
            GetNewAudioItems();

            // Update data for started sources
            GetNewDataForStartedAudioPartSources();

            // Try move not started sources to started sources
            // TryGetData -> if has any data move to started sources
            GetNewDataForNotStartedAudioPartSources();

            // Try start or update data for background audio source
            GetNewDataForBackgroundAudioSource();

            // After all updates try to send something to response
            ProcessCurrentAudioParts();
        } catch (...) {
            Metrics_.SetError("badchunk");
            OnError(TStringBuilder() << "Failed to process new chunk: " << CurrentExceptionMessage(), /* isCritical = */ true);
            return false;
        }

        if (AllAudioPartsAreProcessed()) {
            LogContext_.LogEventInfoCombo<NEvClass::InfoMessage>("All audio items are processed");
        }

        return true;
    }

    bool IsCompleted() override {
        return AllAudioPartsAreProcessed();
    }

    virtual TString GetErrorForIncompleteInput() override {
        return TStringBuilder()
            << "Got end of stream before all audio parts. Processed "
            << AudioPartPtr_
            << "/"
            << AudioPartSource_.size()
        ;
    }

private:
    void ProcessAggregatorRequest() {
        OutputAudioStream_ = CreateOutputAudioStream(
            AggregatorRequest_,
            AhContext_,
            LogContext_,
            Metrics_
        );

        AudioPartSource_.reserve(AggregatorRequest_.GetAudioParts().size());
        AudioPartAudio_.resize(AggregatorRequest_.GetAudioParts().size());
        AudioPartTimings_.resize(AggregatorRequest_.GetAudioParts().size());
        AudioPartCacheKey_.reserve(AggregatorRequest_.GetAudioParts().size());
        AudioPartPtr_ = 0;

        NotStartedAudioSources_.reserve(AggregatorRequest_.GetAudioParts().size());
        for (i32 audioPartId = 0; audioPartId < AggregatorRequest_.GetAudioParts().size(); ++audioPartId) {
            const auto& audioPart = AggregatorRequest_.GetAudioParts()[audioPartId];
            AudioPartSource_.push_back(nullptr);
            AudioPartCacheKey_.push_back(audioPart.GetCacheKey());

            if (audioPart.GetAudioSources().empty()) {
                ythrow yexception() << "Sources not provided for '" << audioPartId << "' audio part";
            }

            TMultiSource multiSource;
            multiSource.Sources_.reserve(audioPart.GetAudioSources().size());
            multiSource.SequenceNumber_ = static_cast<ui32>(audioPartId);
            for (i32 sourceId = 0; sourceId < audioPart.GetAudioSources().size(); ++sourceId) {
                const auto& source = audioPart.GetAudioSources()[sourceId];
                if (source.GetSourceTypeCase() == NTts::TAggregatorRequest::TAudioSource::SOURCETYPE_NOT_SET) {
                    ythrow yexception() << "Source type not set for '" << audioPartId << "' audio part for '" << sourceId << "' source id";
                }

                multiSource.Sources_.push_back(
                    {
                        .Source_ = CreateAudioSource(
                            source,
                            audioPart.GetCacheKey(),
                            LogContext_,
                            Metrics_
                        ),
                        .SequenceNumber_ = static_cast<ui32>(sourceId),
                    }
                );
            }
            NotStartedAudioSources_.push_back(std::move(multiSource));
        }

        if (AggregatorRequest_.GetBackgroundAudio().GetSourceTypeCase() != NTts::TAggregatorRequest::TAudioSource::SOURCETYPE_NOT_SET) {
            BackgroundAudioSource_ = CreateAudioSource(
                AggregatorRequest_.GetBackgroundAudio(),
                // TODO(chegoryu) Do it in better way
                "fake_cache_key",
                LogContext_,
                Metrics_
            );
        }
    }

    void GetNewCacheGetResponses() {
        const auto ttsCacheGetResponseItemRefs = AhContext_->GetProtobufItemRefs(ITEM_TYPE_TTS_CACHE_GET_RESPONSE, NAppHost::EContextItemSelection::Input);

        for (const auto& ttsCacheGetResponseItemRef : ttsCacheGetResponseItemRefs) {
            try {
                NTts::TCacheGetResponse cacheGetResponse;
                ParseProtobufItem(ttsCacheGetResponseItemRef, cacheGetResponse);

                // Even if the response is bad, we will probably get a good response from another source
                // so this is not critical error for now
                bool isResponseBad = (cacheGetResponse.GetStatus() == NTts::ECacheGetResponseStatus::ERROR);

                Metrics_.PushRate("cachegetresponse", NTts::ECacheGetResponseStatus_Name(cacheGetResponse.GetStatus()), ToString(IAudioSource::EType::TTS_CACHE));
                {
                    NTts::TCacheGetResponse cacheGetResponseCopy = cacheGetResponse;
                    cacheGetResponseCopy.MutableCacheEntry()->ClearAudio();
                    if (isResponseBad) {
                        LogContext_.LogEventErrorCombo<NEvClass::RecvFromAppHostTtsCacheGetResponse>(
                            cacheGetResponseCopy.ShortUtf8DebugString(),
                            cacheGetResponse.GetCacheEntry().GetAudio().size()
                        );
                    } else {
                        LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostTtsCacheGetResponse>(
                            cacheGetResponseCopy.ShortUtf8DebugString(),
                            cacheGetResponse.GetCacheEntry().GetAudio().size()
                        );
                    }
                }
                CacheGetResponses_[cacheGetResponse.GetKey()] = std::move(cacheGetResponse);
            } catch (...) {
                Metrics_.PushRate("parse", "error", ToString(IAudioSource::EType::TTS_CACHE));
                LogContext_.LogEventErrorCombo<NEvClass::ErrorMessage>("Failed to parse tts cache get response");
            }
        }
    }

    void GetNewAudioItems() {
        const auto audioItemRefs = AhContext_->GetProtobufItemRefs(ITEM_TYPE_AUDIO, NAppHost::EContextItemSelection::Input);

        for (const auto& audioItemRef : audioItemRefs) {
            try {
                NAliceProtocol::TAudio audio;
                ParseProtobufItem(audioItemRef, audio);

                // As old code assume that empty responseCode is 200 code
                ui32 responseCode = 200;
                if (audio.GetTtsBackendResponse().GetGenerateResponse().has_responsecode() && audio.GetTtsBackendResponse().GetGenerateResponse().responsecode()) {
                    responseCode = audio.GetTtsBackendResponse().GetGenerateResponse().responsecode();
                }

                // Even if the response is bad, we will probably get a good response from another source
                // so this is not critical error for now
                bool isResponseBad = (responseCode != 200);

                Metrics_.PushRate("audio", ToString(responseCode), ToString(IAudioSource::EType::TTS_BACKEND));
                if (audio.HasChunk()) {
                    NAliceProtocol::TAudio audioExceptChunk = audio;
                    audioExceptChunk.ClearChunk();
                    if (isResponseBad) {
                        LogContext_.LogEventErrorCombo<NEvClass::RecvFromAppHostAudioChunk>(audio.GetChunk().GetData().size(), audioExceptChunk.ShortUtf8DebugString());
                    } else {
                        LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostAudioChunk>(audio.GetChunk().GetData().size(), audioExceptChunk.ShortUtf8DebugString());
                    }
                } else {
                    if (isResponseBad) {
                        LogContext_.LogEventErrorCombo<NEvClass::RecvFromAppHostAudio>(audio.ShortUtf8DebugString());
                    } else {
                        LogContext_.LogEventInfoCombo<NEvClass::RecvFromAppHostAudio>(audio.ShortUtf8DebugString());
                    }
                }

                AudioItems_[audio.GetTtsBackendResponse().GetReqSeqNo()].push_back(std::move(audio));
            } catch (...) {
                Metrics_.PushRate("parse", "error", ToString(IAudioSource::EType::TTS_BACKEND));
                LogContext_.LogEventErrorCombo<NEvClass::ErrorMessage>("Failed to parse audio item");
            }
        }
    }

    void GetNewDataForBackgroundAudioSource() {
        if (
            BackgroundAudioSource_ &&
            (BackgroundAudioSource_->GetState() == IAudioSource::STREAM_NOT_STARTED || BackgroundAudioSource_->GetState() == IAudioSource::STREAM_STARTED)
        ) {
            IAudioSource::EState newState = BackgroundAudioSource_->SafeTryReceiveData(
                AhContext_,
                CacheGetResponses_,
                AudioItems_
            );

            switch (newState) {
                case IAudioSource::EState::STREAM_NOT_STARTED:
                case IAudioSource::EState::STREAM_STARTED:
                case IAudioSource::EState::STREAM_FINISHED: {
                    break;
                }
                case IAudioSource::EState::ERROR:
                case IAudioSource::EState::NO_DATA_IN_RESPONSE: {
                    ythrow yexception()
                        << "State of audio source of background audio is '" << ToString(newState)
                        << "'. Error: '" << BackgroundAudioSource_->GetErrorMessage() << "'"
                    ;
                }
            }
        }
    }

    void GetNewDataForStartedAudioPartSources() {
        for (ui32 audioPartId = 0; audioPartId < AudioPartSource_.size(); ++audioPartId) {
            auto& source = AudioPartSource_[audioPartId];
            if (source != nullptr && source->GetState() == IAudioSource::EState::STREAM_STARTED) {
                IAudioSource::EState newState = source->SafeTryReceiveData(
                    AhContext_,
                    CacheGetResponses_,
                    AudioItems_
                );

                switch (newState) {
                    case IAudioSource::EState::STREAM_STARTED:
                    case IAudioSource::EState::STREAM_FINISHED: {
                        break;
                    }
                    case IAudioSource::EState::STREAM_NOT_STARTED:
                    case IAudioSource::EState::ERROR:
                    case IAudioSource::EState::NO_DATA_IN_RESPONSE: {
                        ythrow yexception()
                            << "State of audio source of '" << audioPartId
                            << "' audio part is '" << ToString(newState)
                            << "'. Error: '" << source->GetErrorMessage() << "'"
                        ;
                    }
                }
            }
        }
    }

    void GetNewDataForNotStartedAudioPartSources() {
        TVector<TMultiSource> nextNotStarted;
        for (auto& multiSource : NotStartedAudioSources_) {
            bool started = false;
            TVector<TSource> nextSources;
            for (auto& source : multiSource.Sources_) {
                IAudioSource::EState newState = source.Source_->SafeTryReceiveData(
                    AhContext_,
                    CacheGetResponses_,
                    AudioItems_
                );

                switch (newState) {
                    case IAudioSource::EState::STREAM_NOT_STARTED: {
                        nextSources.push_back(std::move(source));
                        break;
                    }
                    case IAudioSource::EState::STREAM_STARTED:
                    case IAudioSource::EState::STREAM_FINISHED: {
                        AudioPartSource_[multiSource.SequenceNumber_] = std::move(source.Source_);
                        started = true;
                        break;
                    }
                    case IAudioSource::EState::ERROR: {
                        LogContext_.LogEventErrorCombo<NEvClass::ErrorMessage>(
                            TStringBuilder()
                                << "Failed to process '" << multiSource.SequenceNumber_
                                << "' audio part for '"  << source.SequenceNumber_
                                << "' source id: " << source.Source_->GetErrorMessage()
                        );
                        break;
                    }
                    case IAudioSource::EState::NO_DATA_IN_RESPONSE: {
                        break;
                    }
                }

                if (started) {
                    // Do not check other sources
                    break;
                }
            }

            multiSource.Sources_.swap(nextSources);
            if (!started) {
                if (multiSource.Sources_.empty()) {
                    ythrow yexception() << "All sources from '" << multiSource.SequenceNumber_ << "' audio part failed";
                }
                nextNotStarted.push_back(std::move(multiSource));
            }
        }

        NotStartedAudioSources_.swap(nextNotStarted);
    }

    void ProcessCurrentAudioParts() {
        bool needFlush = false;

        if (BackgroundAudioSource_) {
            auto currentState = BackgroundAudioSource_->GetState();
            switch (currentState) {
                case IAudioSource::EState::STREAM_NOT_STARTED:
                case IAudioSource::EState::STREAM_STARTED:
                case IAudioSource::EState::STREAM_FINISHED: {
                    break;
                }
                case IAudioSource::EState::ERROR:
                case IAudioSource::EState::NO_DATA_IN_RESPONSE: {
                    // This case must be unreachable
                    // Must be checked in GetNewDataForBackgroundAudioSource
                    ythrow yexception()
                        << "Something went wrong, state for background audio source is '" << ToString(currentState)
                        << "' but this case must be unreachable"
                    ;
                }
            }

            if (BackgroundAudioSource_->HasNewDataToSend()) {
                IAudioSource::TData data = BackgroundAudioSource_->GetNewDataToSend();
                needFlush |= OutputAudioStream_->AddBackgroundAudioData(data);
            }

            if (currentState != IAudioSource::STREAM_FINISHED) {
                // TODO(VOICESERV-4090, chegoryu) fix this
                // Mvp limitation, we can't process audio parts before BackgroundAudioSource is finished
                if (needFlush) {
                    AhContext_->IntermediateFlush();
                }
                return;
            }
        }

        while (AudioPartPtr_ < AudioPartSource_.size()) {
            auto& source = AudioPartSource_[AudioPartPtr_];
            if (source == nullptr) {
                break;
            }

            auto currentState = source->GetState();
            switch (currentState) {
                case IAudioSource::EState::STREAM_STARTED:
                case IAudioSource::EState::STREAM_FINISHED: {
                    break;
                }
                case IAudioSource::EState::STREAM_NOT_STARTED:
                case IAudioSource::EState::ERROR:
                case IAudioSource::EState::NO_DATA_IN_RESPONSE: {
                    // This case must be unreachable
                    // Must be checked in GetNewDataForStartedSources and GetNewDataForNotStartedSources
                    ythrow yexception()
                        << "Something went wrong, state for '" << AudioPartPtr_
                        << "' audio part is '" << ToString(currentState)
                        << "' but this case must be unreachable"
                    ;
                }
            }

            if (source->HasNewDataToSend()) {
                if (!FirstChunkStatsSent_) {
                    Metrics_.PushHist(StartTime_, "firstchunk", "ok", ToString(source->GetType()));
                    FirstChunkStatsSent_ = true;
                }

                // Get data from source
                IAudioSource::TData data = source->GetNewDataToSend();

                // Add new data to output audio stream
                needFlush |= OutputAudioStream_->AddMainAudioData(data, source->GetType(), CurrentDuration_);

                // Save current audio and timings
                AudioPartAudio_[AudioPartPtr_] += data.Audio_;
                for (const auto& timings : data.Timings_) {
                    AudioPartTimings_[AudioPartPtr_].push_back(timings);
                }
            }

            if (currentState == IAudioSource::STREAM_FINISHED) {
                // Audio source stream finished

                // Send cache save request
                if (AggregatorRequest_.GetEnableSaveToCache()) {
                    needFlush |= SendCacheSetRequest(AudioPartPtr_);
                }

                if (AggregatorRequest_.GetNeedTtsBackendTimings() &&
                    AudioPartTimings_[AudioPartPtr_].empty() &&
                    // No timings for s3 audio is ok
                    source->GetType() != IAudioSource::EType::S3_AUDIO
                ) {
                    Metrics_.PushRate("needbutemptytimings", "warning");
                    LogContext_.LogEventInfoCombo<NEvClass::TWarningMessage>(
                        TStringBuilder()
                            << "Request with NeedTtsBackendTimings but timings is empty for '"
                            << AudioPartPtr_  << "' audio part with '"
                            << ToString(source->GetType()) << "' source type"
                    );
                }

                CurrentDuration_ += source->GetTotalDuration();
                ++AudioPartPtr_;
            } else {
                // Audio source stream not finished
                // Wait for new data
                break;
            }
        }

        // Send EndStream if needed
        if (AllAudioPartsAreProcessed()) {
            if (!FirstChunkStatsSent_) {
                Metrics_.PushHist(StartTime_, "firstchunk", "error", "unknown");
                FirstChunkStatsSent_ = true;
            }

            needFlush = true;
            // Finalize output audio stream
            OutputAudioStream_->Finalize();
        }

        if (needFlush) {
            AhContext_->IntermediateFlush();
        }
    }

    bool SendCacheSetRequest(ui32 audioPartPtr) {
        if (const auto& source = AudioPartSource_[audioPartPtr]; source->GetType() == IAudioSource::EType::TTS_BACKEND) {
            if (const auto& cacheKey = AudioPartCacheKey_[audioPartPtr]; AlreadySavedToCache_.insert(cacheKey).second) {
                const auto& audio = AudioPartAudio_[audioPartPtr];
                const auto& timings = AudioPartTimings_[audioPartPtr];

                if (timings.empty()) {
                    Metrics_.PushRate("emptytimingscacheset", "warning");
                    LogContext_.LogEventInfoCombo<NEvClass::WarningMessage>(
                        TStringBuilder()
                            << "Save audio with empty timings to cache with key '"
                            << cacheKey << "' for '" << audioPartPtr
                            << "' audio part"
                    );
                }

                NTts::TCacheSetRequest cacheSetRequest;
                cacheSetRequest.SetKey(cacheKey);
                {
                    auto& cacheEntry = *cacheSetRequest.MutableCacheEntry();
                    cacheEntry.SetAudio(audio);

                    for (const auto& subtimings : timings) {
                        cacheEntry.AddTimings()->CopyFrom(subtimings);
                    }

                    cacheEntry.SetDuration(source->GetTotalDuration().SecondsFloat());
                }

                AhContext_->AddProtobufItem(cacheSetRequest, ITEM_TYPE_TTS_CACHE_SET_REQUEST);
                {
                    NTts::TCacheSetRequest cacheSetRequestCopy = cacheSetRequest;
                    cacheSetRequestCopy.MutableCacheEntry()->ClearAudio();
                    LogContext_.LogEventInfoCombo<NEvClass::SendToAppHostTtsCacheSetRequest>(
                        cacheSetRequestCopy.ShortUtf8DebugString(),
                        cacheSetRequest.GetCacheEntry().GetAudio().size()
                    );
                }

                Metrics_.PushRate("cacheset", "ok");
                return true;
            } else {
                LogContext_.LogEventInfoCombo<NEvClass::InfoMessage>(TStringBuilder() << "Audio part with cache key '" << cacheKey << "' already saved to cache");
                Metrics_.PushRate("samecacheset", "ok");
                return false;
            }
        } else {
            return false;
        }
    }

    bool AllAudioPartsAreProcessed() {
        return (AudioPartPtr_ == AudioPartSource_.size());
    }

private:
    static constexpr TStringBuf SOURCE_NAME = "tts_aggregator";

    // Sum of the durations from responses meta info
    // If some of responses has no meta info, duration of this response
    // assumed to equal zero
    // Need only to calculate TTS.Timings in the same way that it's done
    // in python uniproxy
    TDuration CurrentDuration_ = TDuration::Zero();

    NTts::TAggregatorRequest AggregatorRequest_;

    TOutputAudioStreamPtr OutputAudioStream_;

    // Sources that will be used for the response
    TVector<TAudioSourcePtr> AudioPartSource_;
    TVector<TString> AudioPartAudio_;
    TVector<TVector<TTS::GenerateResponse::Timings>> AudioPartTimings_;
    TVector<TString> AudioPartCacheKey_;
    ui32 AudioPartPtr_ = 0;
    bool FirstChunkStatsSent_ = false;

    TAudioSourcePtr BackgroundAudioSource_ = nullptr;

    // As soon as some non-(empty/error) data appears in one of the subsources of multisource, it moves
    // to AudioPartSources, all other sources from this multisource are ignored
    TVector<TMultiSource> NotStartedAudioSources_;

    THashSet<TString> AlreadySavedToCache_;

    // CacheKey -> TCacheGetResponse
    THashMap<TString, NTts::TCacheGetResponse> CacheGetResponses_;
    // ReqSeqNo -> AudioItems
    THashMap<ui32, TVector<NAliceProtocol::TAudio>> AudioItems_;
};

} // namespace

NThreading::TPromise<void> TtsAggregator(NAppHost::TServiceContextPtr ctx, TLogContext logContext) {
    TIntrusivePtr<TTtsAggregator> stream(new TTtsAggregator(ctx, std::move(logContext)));
    stream->OnNextInput();
    return stream->GetFinishPromise();
}

}  // namespace NAlice::NCuttlefish::NAppHostServices
