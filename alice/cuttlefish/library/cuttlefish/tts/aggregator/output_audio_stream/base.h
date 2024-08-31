#pragma once

#include <alice/cuttlefish/library/cuttlefish/tts/aggregator/audio_source/base.h>
#include <voicetech/library/mime/mime_type.h>
#include <voicetech/library/ogg/opus_stream.h>

namespace NAlice::NCuttlefish::NAppHostServices {

struct TOutputAudioStreamParams {
    NAppHost::TServiceContextPtr AhContext_;

    TLogContext LogContext_;
    // WARNING: Intended reference here
    // You must keep metrics alive for the rest of the object life
    TSourceMetrics& Metrics_;
    const TString Mime_;
    const bool NeedTtsBackendTimings_;
    const bool NeedRtsTimings_;
    const TDuration RtsBuffer_;
};

class IOutputAudioStream: protected TOutputAudioStreamParams {
public:
    class TTimingsBuilder {
    public:
        TTimingsBuilder(TOutputAudioStreamParams& params)
            : PageReader{new NVoicetech::NOgg::TOggPageReader{}}
            , Params_(params)
            , RtsBuffer_(params.RtsBuffer_)
        {}
        void BuildTimings(TStringBuf data, NAliceProtocol::TAudioChunk::TTimings&);
        bool IsBroken() const noexcept {
            return Broken_;
        }

    private:
        void BuildTimingsNotSafe(TStringBuf data, NAliceProtocol::TAudioChunk::TTimings&);

        THolder<NVoicetech::NOgg::TOggPageReader> PageReader;
        TMaybe<NVoicetech::NOgg::TOpusStreamReader> OpusStreamReader;
        TOutputAudioStreamParams& Params_;
        TDuration RtsBuffer_;
        bool Broken_{false};
    };


    IOutputAudioStream(const TOutputAudioStreamParams& params)
        : TOutputAudioStreamParams(params)
        , OggOpus_(MimeIsOpus())
    {
    }
    virtual ~IOutputAudioStream() = default;

    // Add new data to main stream and pop some chunks to output
    // Returns true if something was added to output audio stream
    bool AddMainAudioData(
        const IAudioSource::TData& data,
        IAudioSource::EType sourceType,
        TDuration timingsOffset
    );

    // Add new data to background audio and pop some chunks to output
    // Returns true if something was added to output audio stream
    bool AddBackgroundAudioData(const IAudioSource::TData& data);

    // Pop all remain chunks to output
    void Finalize();

protected:
    virtual bool AddMainAudioDataImpl(
        const IAudioSource::TData& data,
        IAudioSource::EType sourceType,
        TDuration timingsOffset
    ) = 0;
    virtual bool AddBackgroundAudioDataImpl(const IAudioSource::TData& data) = 0;
    // Pop all remain chunks to output
    virtual void FinalizeImpl() = 0;

    void SendBeginStream(IAudioSource::EType firstChunkSourceType);
    void SendEndStream();
    bool SendAudio(const TString& audio, IAudioSource::EType sourceType);
    bool SendTimings(
        const TVector<TTS::GenerateResponse::Timings>& timings,
        IAudioSource::EType sourceType,
        TDuration timingsOffset
    );

private:
    bool MimeIsOpus() {
        try {
            NVoicetech::NASConv::TMimeType mimeType{Mime_};
            if (mimeType.Format() == "ogg" && mimeType.Codec() == "libopus") {
                return true;
            }
        } catch (...) {
            // ignore bad|unknown formats
        }
        return false;
    }

protected:
    bool StreamStarted_ = false;

private:
    const bool OggOpus_;
    THolder<TTimingsBuilder> TimingsBuilder_;
};

using TOutputAudioStreamPtr = THolder<IOutputAudioStream>;

TOutputAudioStreamPtr CreateOutputAudioStream(
    const NTts::TAggregatorRequest& aggregatorRequest,
    NAppHost::TServiceContextPtr ahContext,
    const TLogContext& logContext,
    TSourceMetrics& metrics
);

}  // namespace NAlice::NCuttlefish::NAppHostServices
