#include "service.h"
#include "util.h"

#include <alice/cuttlefish/library/apphost/item_parser.h>
#include <alice/cuttlefish/library/cuttlefish/common/exp_flags.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/cuttlefish/common/metrics.h>
#include <alice/cuttlefish/library/protos/asr.pb.h>
#include <alice/cuttlefish/library/protos/audio.pb.h>
#include <alice/cuttlefish/library/protos/session.pb.h>
#include <alice/cuttlefish/library/protos/store_audio.pb.h>

#include <apphost/lib/proto_answers/http.pb.h>

#include <voicetech/library/common/wav_header.h>

namespace NAlice::NCuttlefish::NAppHostServices {

namespace {

constexpr int WAV_HEADER_LENGTH = 44;

NAppHostHttp::THttpRequest ConstructMdsHttpRequest(const NAliceCuttlefishConfig::TConfig& config, const NAliceProtocol::TRequestContext& reqCtx, const TBuffer& content, bool isSpotter) {
    TString filename = ConstructMdsFilename(reqCtx, isSpotter);
    TDuration ttl = config.mds().ttl();
    if (NExpFlags::ConductingExperiment(reqCtx, "log_spotter_diff")) {
        filename = "test_" + filename;
        ttl = TDuration::Days(3);
    }

    NAppHostHttp::THttpRequest mdsHttpRequest;
    mdsHttpRequest.SetPath(TStringBuilder{} << "/upload-" << config.mds().upload_namespace() << "/" << filename << "?expire=" << ttl.Seconds() << "s");
    mdsHttpRequest.SetMethod(NAppHostHttp::THttpRequest_EMethod_Post);
    mdsHttpRequest.SetContent(content.Data(), content.Size());
    return mdsHttpRequest;
}

TBuffer ConstructAudioBuffer(const TString& format) {
    TBuffer buffer;
    if (format.Contains("pcm")) {
        // make space for WAV header in future
        buffer.Resize(WAV_HEADER_LENGTH);
    }
    return buffer;
}

void ConstructWavHeader(const TString& format, TBuffer& buffer) {
    if (format.Contains("pcm")) {
        int sampleRate;
        if (format.Contains("48")) {
            sampleRate = 48000;
        } else if (format.Contains("8")) {
            sampleRate = 8000;
        } else {
            sampleRate = 16000;
        }

        TMemoryOutput out{buffer.Data(), WAV_HEADER_LENGTH};
        out << NVoicetech::NCommon::GenerateWavHeaderWithCustomChannels(sampleRate, ChannelsFromMime(format), buffer.Size() - WAV_HEADER_LENGTH);
    }
}

void RetrieveMdsHttpAnswer(NAppHost::IServiceContext& ctx, const NAppHostHttp::THttpResponse& mdsResp, const bool isSpotter) {
    NAliceProtocol::TStoreAudioResponse response;
    response.SetStatusCode(mdsResp.GetStatusCode());
    response.SetIsSpotter(isSpotter);
    if (mdsResp.GetStatusCode() == 200) {
        response.SetKey(GetKeyFromMdsSaveResponse(mdsResp.GetContent()));
    }

    if (isSpotter) {
        ctx.AddProtobufItem(response, ITEM_TYPE_STORE_AUDIO_RESPONSE_SPOTTER);
    } else {
        ctx.AddProtobufItem(response, ITEM_TYPE_STORE_AUDIO_RESPONSE);
    }
}


class TStoreAudioPre : public TThrRefBase {
public:
    TStoreAudioPre(const NAliceCuttlefishConfig::TConfig& config, NAppHost::TServiceContextPtr ctx, TLogContext logContext)
        : Config(config)
        , AhContext(ctx)
        , LogContext(logContext)
        , Promise(NThreading::NewPromise())
        , Metrics(*ctx, "store_audio_pre")
        , RequestContext(AhContext->GetOnlyProtobufItem<NAliceProtocol::TRequestContext>(ITEM_TYPE_REQUEST_CONTEXT))
    {
        LogContext.LogEventInfoCombo<NEvClass::InfoMessage>(TStringBuilder() << "StoreAudioPre request for messageId " << RequestContext.GetHeader().GetMessageId());

        if (!RequestContext.GetAudioOptions().HasFormat()) {
            RequestContext.MutableAudioOptions()->SetFormat("pcm16");
        }

        Buffer = ConstructAudioBuffer(RequestContext.GetAudioOptions().GetFormat());
    }

    void OnNextInput() {
        const auto items = AhContext->GetProtobufItemRefs(NAppHost::EContextItemSelection::Input);
        for (auto it = items.begin(); it != items.end(); ++it) {
            const auto type = it.GetType();
            if (type == ITEM_TYPE_AUDIO) {
                try {
                    NAliceProtocol::TAudio audio;
                    ParseProtobufItem(*it, audio);
                    if (!OnAudio(std::move(audio))) {
                        return;
                    }
                } catch (...) {
                    LogContext.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << "OnAudio failed " << CurrentExceptionMessage());
                    return;
                }
            } else if (type == ITEM_TYPE_ASR_FINISHED) {
                try {
                    NAliceProtocol::TAsrFinished asrFinished;
                    ParseProtobufItem(*it, asrFinished);
                    OnAsrFinished(asrFinished);
                    return;
                } catch(...) {
                    LogContext.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << "OnAsrFinished failed " << CurrentExceptionMessage());
                    return;
                }
            }
        }

        // continue consuming input
        TIntrusivePtr<TStoreAudioPre> self(this);
        AhContext->NextInput().Apply([stream = std::move(self)](auto hasData) mutable {
            if (!hasData.GetValue()) {
                stream->OnEndInput();
                stream->Promise.SetValue();
                return;
            }
            stream->OnNextInput();
        });
    }

    void OnEndInput() {
        if (Buffer.Size() > WAV_HEADER_LENGTH) {
            // we received at least one not empty chunk but no stream control
            // https://st.yandex-team.ru/VOICESERV-4420#62cea7e19564827ce5c1fdf4
            // will be fixed in https://st.yandex-team.ru/SK-6239
            Metrics.PushRate("stream_control", "not_received");
            LogContext.LogEventInfoCombo<NEvClass::StreamControlIsAbsentInStoreAudioPreWarning>();
            SendBufferStreamToMds();
        }
    }

    bool OnAudio(NAliceProtocol::TAudio audio) {
        if (audio.HasChunk()) {
            // Write to buffer
            const auto& data = audio.GetChunk().GetData();
            Buffer.Append(data.data(), data.size());
            Metrics.PushRate("audio_chunk");
        }

        if (audio.HasEndSpotter()) {
            // Send buffer to MDS
            ConstructWavHeader(RequestContext.GetAudioOptions().GetFormat(), Buffer);
            AhContext->AddProtobufItem(ConstructMdsHttpRequest(Config, RequestContext, Buffer, /* isSpotter = */ true), ITEM_TYPE_MDS_STORE_SPOTTER_HTTP_REQUEST);
            LogContext.LogEventInfoCombo<NEvClass::InfoMessage>(TStringBuilder() << "Save spotter MDS audiofile with size " << Buffer.size() << " bytes");

            Metrics.PushRate("end_of_spotter");
            Metrics.PushRate(Buffer.size(), "spotter_size");

            // Construct buffer again
            Buffer = ConstructAudioBuffer(RequestContext.GetAudioOptions().GetFormat());
        }

        if (audio.HasEndStream()) {
            SendBufferStreamToMds();
            Promise.SetValue();
            // Don't construct buffer again - no audio chunks will come next
            return false;
        }

        return true;
    }

    void OnAsrFinished(const NAliceProtocol::TAsrFinished& asrFinished) {
        LogContext.LogEventInfoCombo<NEvClass::RecvFromAppHostAsrFinished>(asrFinished.ShortUtf8DebugString());
        SendBufferStreamToMds();
        Promise.SetValue();
    }

    void SendBufferStreamToMds() {
        ConstructWavHeader(RequestContext.GetAudioOptions().GetFormat(), Buffer);
        AhContext->AddProtobufItem(ConstructMdsHttpRequest(Config, RequestContext, Buffer, /* isSpotter = */ false), ITEM_TYPE_MDS_STORE_STREAM_HTTP_REQUEST);
        LogContext.LogEventInfoCombo<NEvClass::InfoMessage>(TStringBuilder() << "Save stream MDS audiofile with size " << Buffer.size() << " bytes");

        Metrics.PushRate("end_of_stream");
        Metrics.PushRate(Buffer.size(), "audio_size");
    }

private:
    const NAliceCuttlefishConfig::TConfig& Config;
    NAppHost::TServiceContextPtr AhContext;
    TLogContext LogContext;

public:
    NThreading::TPromise<void> Promise;

private:
    TSourceMetrics Metrics;
    NAliceProtocol::TRequestContext RequestContext;
    TBuffer Buffer;
};


class TStoreAudioPost : public TThrRefBase {
public:
    TStoreAudioPost(NAppHost::TServiceContextPtr ctx, TLogContext logContext)
        : AhContext(ctx)
        , Metrics(*ctx, "store_audio_post")
        , LogContext(logContext)
        , Promise(NThreading::NewPromise())
    {
    }

    void OnNextInput() {
        const auto items = AhContext->GetProtobufItemRefs(NAppHost::EContextItemSelection::Input);
        for (auto it = items.begin(); it != items.end(); ++it) {
            const auto type = it.GetType();

            for (const auto& [itemType, isSpotter] : TVector<std::pair<TStringBuf, bool>>{
                {ITEM_TYPE_MDS_STORE_SPOTTER_HTTP_RESPONSE, true},
                {ITEM_TYPE_MDS_STORE_STREAM_HTTP_RESPONSE, false},
            }) {
                if (type == itemType) {
                    try {
                        NAppHostHttp::THttpResponse mdsResp;
                        ParseProtobufItem(*it, mdsResp);
                        LogContext.LogEventInfoCombo<NEvClass::InfoMessage>(TStringBuilder() << "MDS " << type << " response: " << mdsResp);
                        RetrieveMdsHttpAnswer(*AhContext, mdsResp, isSpotter);
                        Metrics.RateHttpCode(mdsResp.GetStatusCode(), "mds");
                    } catch (...) {
                        LogContext.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << "Filed to process MDS response " << CurrentExceptionMessage());
                        Metrics.PushRate("response", "error", "mds");
                    }
                }
            }
        }

        // continue consuming input
        TIntrusivePtr<TStoreAudioPost> self(this);
        AhContext->NextInput().Apply([stream = std::move(self)](auto hasData) mutable {
            if (!hasData.GetValue()) {
                stream->Promise.SetValue();
                return;
            }
            stream->OnNextInput();
        });
    }

private:
    NAppHost::TServiceContextPtr AhContext;
    TSourceMetrics Metrics;
    TLogContext LogContext;

public:
    NThreading::TPromise<void> Promise;
};

} // namespace

NThreading::TPromise<void> StoreAudioPre(const NAliceCuttlefishConfig::TConfig& config, NAppHost::TServiceContextPtr ctx, TLogContext logContext) {
    TIntrusivePtr<TStoreAudioPre> stream(new TStoreAudioPre(config, ctx, std::move(logContext)));
    stream->OnNextInput();
    return stream->Promise;
}

NThreading::TPromise<void> StoreAudioPost(NAppHost::TServiceContextPtr ctx, TLogContext logContext) {
    TIntrusivePtr<TStoreAudioPost> stream(new TStoreAudioPost(ctx, std::move(logContext)));
    stream->OnNextInput();
    return stream->Promise;
}

}  // namespace NAlice::NCuttlefish::NAppHostServices
