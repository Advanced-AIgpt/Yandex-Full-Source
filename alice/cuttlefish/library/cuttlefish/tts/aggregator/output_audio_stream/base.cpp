#include "base.h"

#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>

#include <voicetech/library/aproc/merge.h>
#include <voicetech/library/ogg/opus_stream.h>

using namespace NVoicetech::NOgg;

namespace NAlice::NCuttlefish::NAppHostServices {

namespace {

class TTransparentOutputAudioStream : public IOutputAudioStream {
public:
    TTransparentOutputAudioStream(const TOutputAudioStreamParams& params)
        : IOutputAudioStream(params)
    {}

    bool AddMainAudioDataImpl(
        const IAudioSource::TData& data,
        IAudioSource::EType sourceType,
        TDuration timingsOffset
    ) override {
        bool needFlush = false;

        needFlush |= SendAudio(data.Audio_, sourceType);
        needFlush |= SendTimings(data.Timings_, sourceType, timingsOffset);

        return needFlush;
    }

    bool AddBackgroundAudioDataImpl(const IAudioSource::TData& data) override {
        Y_UNUSED(data);

        Metrics_.PushRate("unexpectedbackgroundaudio", "error");
        LogContext_.LogEventErrorCombo<NEvClass::ErrorMessage>("Unexpected background audio data for transparent stream");

        return false;
    }

    void FinalizeImpl() override {
    }
};

class TOutputAudioStreamWithBackground : public IOutputAudioStream {
public:
    TOutputAudioStreamWithBackground(const TOutputAudioStreamParams& params)
        : IOutputAudioStream(params)
    {}

    bool AddMainAudioDataImpl(
        const IAudioSource::TData& data,
        IAudioSource::EType sourceType,
        TDuration timingsOffset
    ) override {
        bool needFlush = false;

        if (!Merger_.Defined()) {
            Merger_.ConstructInPlace(BackgroundAudioData_);
        }

        needFlush |= PushDataToMerger(data.Audio_);
        needFlush |= SendTimings(data.Timings_, sourceType, timingsOffset);

        return needFlush;
    }

    bool AddBackgroundAudioDataImpl(const IAudioSource::TData& data) override {
        if (!StreamStarted_) {
            BackgroundAudioData_ += data.Audio_;
        } else {
            // TODO(VOICESERV-4090, chegoryu): fix this
            // Mvp limitaion
            Metrics_.PushRate("backgroundaudioafterstreamstart", "error");
            LogContext_.LogEventErrorCombo<NEvClass::ErrorMessage>("Got background audio after start of the stream");
        }

        return false;
    }

    void FinalizeImpl() override {
        SendFinalWithAttenuation();
    }

private:
    bool PushDataToMerger(const TString& audio) {
        Merger_->PushChunk(audio);

        if (const size_t readySamplesToMerge = Merger_->ReadySamplesToMerge(); readySamplesToMerge >= SAMPLES_TO_POP_DIVISOR) {
            return SendAudio(Merger_->PopMerged(readySamplesToMerge - readySamplesToMerge % SAMPLES_TO_POP_DIVISOR, /* finish = */ false), IAudioSource::EType::MERGED);
        } else {
            return false;
        }
    }

    void SendFinalWithAttenuation() {
        size_t readySamplesToMerge = Merger_->ReadySamplesToMerge();
        size_t backgroundSamplesLeft = Merger_->BackgroundSamplesLeft();

        // Attenuation is impossible because here we have less than SAMPLES_TO_POP_DIVISOR samples
        // 20ms of attenuation will do nothing here
        if (backgroundSamplesLeft <= readySamplesToMerge) {
            SendAudio(Merger_->PopMerged(readySamplesToMerge, /* finish = */ true), IAudioSource::EType::MERGED);
            return;
        }

        TStringBuilder result;

        const size_t popCount = (Min(backgroundSamplesLeft, MAX_FINAL_BACKGROUND_SAMPLES) + SAMPLES_TO_POP_DIVISOR - 1) / SAMPLES_TO_POP_DIVISOR;
        double backgroundMultiplier = 1.0;
        for (size_t i = 0; i < popCount; ++i) {
            result << Merger_->PopMerged(SAMPLES_TO_POP_DIVISOR, /* finish = */ false, backgroundMultiplier, /* allowEmptySamples = */ true);
            backgroundMultiplier = Max(backgroundMultiplier - BACKGROUND_ATTENUATION_RATE, 0.0);
        }
        result << Merger_->PopMerged(0, true);

        SendAudio(ToString(result), IAudioSource::EType::MERGED);
    }

private:
    // TODO(VOICESERV-4090, chegoryu): fix this
    // Mvp limitaion: number of samples must be divisible by 960 ((48000 rate) * 0.02s = 960)
    static constexpr size_t SAMPLES_TO_POP_DIVISOR = 960;

    // WARNING: Constants chosen by eye to sound good
    static constexpr size_t MAX_FINAL_BACKGROUND_SAMPLES = 48000 * 5;
    // Always same attenuation rate
    // Even if we have less samples (because at the end background has attenuation itself)
    static constexpr double BACKGROUND_ATTENUATION_RATE = (double)SAMPLES_TO_POP_DIVISOR / MAX_FINAL_BACKGROUND_SAMPLES;

    TString BackgroundAudioData_;
    TMaybe<NVoicetech::NAudio::TMerger> Merger_;
};

} // namespace

bool IOutputAudioStream::AddMainAudioData(
    const IAudioSource::TData& data,
    IAudioSource::EType sourceType,
    TDuration timingsOffset
) {
    bool needFlush = false;

    if (!StreamStarted_) {
        needFlush = true;
        SendBeginStream(sourceType);
    }

    needFlush |= AddMainAudioDataImpl(data, sourceType, timingsOffset);

    return needFlush;

}

bool IOutputAudioStream::AddBackgroundAudioData(const IAudioSource::TData& data) {
    return AddBackgroundAudioDataImpl(data);
}

void IOutputAudioStream::Finalize() {
    if (!StreamStarted_) {
        SendBeginStream(IAudioSource::EType::NOT_SET);
        Metrics_.PushRate("emptyoutput", "error");
        LogContext_.LogEventErrorCombo<NEvClass::ErrorMessage>("Stream finished without any data");
    }

    FinalizeImpl();

    SendEndStream();
}

void IOutputAudioStream::SendBeginStream(IAudioSource::EType firstChunkSourceType) {
    StreamStarted_ = true;
    NAliceProtocol::TAudio audio;
    audio.MutableBeginStream()->SetMime(Mime_);
    audio.MutableTtsAggregatorAudioMetaInfo()->SetFirstChunkAudioSource(IAudioSource::TypeToAggregatorAudioMetaInfoEnum(firstChunkSourceType));

    AhContext_->AddProtobufItem(audio, ITEM_TYPE_AUDIO);
    LogContext_.LogEventInfoCombo<NEvClass::SendToAppHostAudio>(audio.ShortUtf8DebugString());
}

void IOutputAudioStream::SendEndStream() {
    NAliceProtocol::TAudio audio;
    audio.MutableEndStream();
    audio.MutableTtsAggregatorAudioMetaInfo();

    AhContext_->AddProtobufItem(audio, ITEM_TYPE_AUDIO);
    LogContext_.LogEventInfoCombo<NEvClass::SendToAppHostAudio>(audio.ShortUtf8DebugString());
}

void IOutputAudioStream::TTimingsBuilder::BuildTimings(TStringBuf data, NAliceProtocol::TAudioChunk::TTimings& timings) {
    if (Broken_) {
        return;
    }

    try {
        BuildTimingsNotSafe(data, timings);
    } catch (...) {
        Broken_ = true;
        Params_.Metrics_.PushRate("timings_builder", "error");
        Params_.LogContext_.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << "timings_builder error: " << CurrentExceptionMessage());
    }
}

void IOutputAudioStream::TTimingsBuilder::BuildTimingsNotSafe(TStringBuf data, NAliceProtocol::TAudioChunk::TTimings& timings) {
    // fill Timings.Records (if need&can)
    PageReader->Push(data);
    size_t consumedData = 0;
    size_t lastChunkEnd = 0;
    while (TOggPage page = PageReader->Pop()) {
        consumedData += page.GetTotalSize() - page.GetPayloadSize();  // count headers + page_segments
        if (!OpusStreamReader.Defined()) {
            OpusStreamReader.ConstructInPlace(page);
        } else {
            OpusStreamReader->PushPage(page);
        }

        while (TOpusPacket packet = OpusStreamReader->PopOpusPacket()) {
            consumedData += packet.GetData().Size();
            if (packet.GetType() == TOpusPacket::EType::AUDIO) {
                auto& audioPacket = packet.ToAudioPacket();
                const TOpusAudioPacket::TInfo info = audioPacket.GetInfo();
                NAliceProtocol::TAudioChunk::TTimings::TRecord record;
                record.SetSize(consumedData - lastChunkEnd);
                if (RtsBuffer_ >= info.GetDuration()) {
                    record.SetMilliseconds(0);
                    RtsBuffer_ -= info.GetDuration();
                } else if (RtsBuffer_.GetValue()) {
                    record.SetMilliseconds((info.GetDuration() - RtsBuffer_).MilliSeconds());
                    RtsBuffer_ = TDuration::Zero();
                } else {
                    record.SetMilliseconds(info.GetDuration().MilliSeconds());
                }
                timings.MutableRecords()->Add(std::move(record));
                lastChunkEnd = consumedData;
            }
        }
        if (page.IsEndOfStream()) {
            PageReader.Reset(new NVoicetech::NOgg::TOggPageReader{});
            OpusStreamReader.Clear();
            break;
        }
    }
    if (size_t leftTail = data.Size() - consumedData) {
        // add left data with zero duration
        NAliceProtocol::TAudioChunk::TTimings::TRecord record;
        record.SetSize(leftTail);
        record.SetMilliseconds(0);
        timings.MutableRecords()->Add(std::move(record));
        Params_.Metrics_.PushRate("timings_builder", "has_tail");
        Params_.LogContext_.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << "timings_builder has_tail length=" << leftTail);
    }
}

bool IOutputAudioStream::SendAudio(const TString& audioData, IAudioSource::EType sourceType) {
    if (audioData.empty()) {
        // Do not send empty data
        return false;
    }

    NAliceProtocol::TAudio audio;
    audio.MutableChunk()->SetData(audioData);
    if (NeedRtsTimings_) {
        if (OggOpus_) {
            if (!TimingsBuilder_) {
                TimingsBuilder_.Reset(new TTimingsBuilder(*this));
            }

            if (!TimingsBuilder_->IsBroken()) {
                TimingsBuilder_->BuildTimings(TStringBuf(audioData), *audio.MutableChunk()->MutableTimings());
            }
        }
    }
    audio.MutableTtsAggregatorAudioMetaInfo()->SetCurrentChunkAudioSource(IAudioSource::TypeToAggregatorAudioMetaInfoEnum(sourceType));

    AhContext_->AddProtobufItem(audio, ITEM_TYPE_AUDIO);
    Metrics_.PushRate(audioData.size(), "sent", "ok");
    {
        NAliceProtocol::TAudio audioExceptChunk = audio;
        audioExceptChunk.ClearChunk();
        LogContext_.LogEventInfoCombo<NEvClass::SendToAppHostAudioChunk>(audio.GetChunk().GetData().size(), audioExceptChunk.ShortUtf8DebugString());
    }

    return true;
}

bool IOutputAudioStream::SendTimings(
    const TVector<TTS::GenerateResponse::Timings>& timings,
    IAudioSource::EType sourceType,
    TDuration timingsOffset
) {
    if (!NeedTtsBackendTimings_) {
        return false;
    }

    if (timings.empty()) {
        // Do not send empty timings
        return false;
    }

    // Сopied as is from https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/processors/tts.py?rev=r7978885#L502-514
    NTts::TTimings ttsTimings;

    for (const auto& timing : timings) {
        auto* patchedTiming = ttsTimings.AddTimings();
        patchedTiming->CopyFrom(timing);
        for (auto& subTiming : *patchedTiming->Mutabletimings()) {
            subTiming.Settime(subTiming.Gettime() + timingsOffset.MillisecondsFloat());
        }
    }

    ttsTimings.SetIsFromCache(sourceType == IAudioSource::EType::TTS_CACHE);

    AhContext_->AddProtobufItem(ttsTimings, ITEM_TYPE_TTS_TIMINGS);
    Metrics_.PushRate("timings", "ok");
    LogContext_.LogEventInfoCombo<NEvClass::SendToAppHostTtsTimings>(ttsTimings.ShortUtf8DebugString());

    return true;
}

TOutputAudioStreamPtr CreateOutputAudioStream(
    const NTts::TAggregatorRequest& aggregatorRequest,
    NAppHost::TServiceContextPtr ahContext,
    const TLogContext& logContext,
    TSourceMetrics& metrics
) {
    const TOutputAudioStreamParams params{
        ahContext,
        logContext,
        metrics,
        aggregatorRequest.GetMime(),
        aggregatorRequest.GetNeedTtsBackendTimings(),
        aggregatorRequest.GetNeedRtsTimings(),
        aggregatorRequest.HasRtsBufferSeconds() ? TDuration::Seconds(aggregatorRequest.GetRtsBufferSeconds()) : TDuration::Seconds(5)
    };
    if (aggregatorRequest.GetBackgroundAudio().GetSourceTypeCase() != NTts::TAggregatorRequest::TAudioSource::SOURCETYPE_NOT_SET) {
        return MakeHolder<TOutputAudioStreamWithBackground>(params);
    } else {
        return MakeHolder<TTransparentOutputAudioStream>(params);
    }
}

}  // namespace NAlice::NCuttlefish::NAppHostServices
