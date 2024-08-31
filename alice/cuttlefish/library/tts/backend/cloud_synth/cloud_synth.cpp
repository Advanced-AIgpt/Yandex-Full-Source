#include "cloud_synth.h"

#include <voicetech/asr/cloud_engine/api/speechkit/v3/service/tts/tts_service.grpc.pb.h>
#include <voicetech/library/proto_api/ttsbackend.pb.h>
#include <grpcpp/grpcpp.h>

using namespace NAlice::NTts;
using namespace NAlice::NCloudSynth;
using namespace speechkit::v3;
using namespace speechkit::tts::v3;

namespace {

static const TString VTB_VOICE = "cloud_vtb_brand_voice";
static const TString VTB_VOICE_KEY = "b1g4atvsitpj2411v2nc";

static const TString CLOUD_VOICE_PREFIX = "cloud_";

} // namespace

TCloudSynth::TCloudSynth(
    TIntrusivePtr<TCallbacks> callbacks,
    const TConfig& config,
    NRTLog::TRequestLoggerPtr rtLogger
)
    : TCloudSynth::TInterface(callbacks)
    , Metrics_(TCloudSynth::SOURCE_NAME)
    , Config_(config)
    , RtLogger_(rtLogger)
{}

void TCloudSynth::ProcessBackendRequest(const NProtobuf::TBackendRequest& backendRequest) {
    if (Cancelled_) {
        Metrics_.PushRate("backendrequest", "canceled");
        return;
    }
    Callbacks_->OnStartRequestProcessing(backendRequest.GetReqSeqNo());
    if (!backendRequest.HasGenerate()) {
        Metrics_.PushRate("backendrequest", "invalid");
        Callbacks_->OnInvalidRequest(backendRequest.GetReqSeqNo(), "No generate request in tts backend request");
        return;
    }
    Metrics_.PushRate("backendrequest", "added");
    NTts::TSourceMetrics requestMetrics("cloud_synth_handler");

    NAlice::NCuttlefish::TRTLogActivation rtLogActivation =
        RtLogger_
        ? NAlice::NCuttlefish::TRTLogActivation(
            RtLogger_,
            TStringBuilder() << "cloud-synth-" << backendRequest.GetReqSeqNo(),
            /* newRequest = */ false
        )
        : NAlice::NCuttlefish::TRTLogActivation()
    ;
    try {
        const auto& generate = backendRequest.GetGenerate();
        UtteranceSynthesisRequest request;
        request.set_text(generate.text());
        // need unsafe_mode for long texts support
        request.set_unsafe_mode(true);

        auto token = Config_.LegacyOauthToken_;

        if (!generate.voices().empty()) {
            TString voice = generate.voices(0).name();
            if (voice == VTB_VOICE) {
                request.add_hints()->set_voice(VTB_VOICE_KEY);
            } else if (voice.StartsWith(CLOUD_VOICE_PREFIX)) {
                voice = voice.substr(CLOUD_VOICE_PREFIX.size());
                auto tokenPtr = Config_.VoiceToToken_.FindPtr(voice);
                if (!tokenPtr) {
                    Metrics_.PushRate("backendrequest", "invalid");
                    Callbacks_->OnInvalidRequest(backendRequest.GetReqSeqNo(), TStringBuilder() << "Unsupported voice: " << voice);
                    rtLogActivation.Finish(/* ok = */ false);
                    return;
                }
                token = *tokenPtr;
                request.add_hints()->set_voice(voice);
            } else {
                Metrics_.PushRate("backendrequest", "invalid");
                Callbacks_->OnInvalidRequest(backendRequest.GetReqSeqNo(), TStringBuilder() << "Unsupported voice: " << voice);
                rtLogActivation.Finish(/* ok = */ false);
                return;
            }
        }
        if (generate.has_speed()) {
            request.add_hints()->set_speed(generate.speed());
        }
        if (generate.volume()) {
            request.add_hints()->set_volume(generate.volume());
        }
        if (generate.has_mime()) {
            AudioFormatOptions *format = request.mutable_output_audio_spec();
            if (generate.mime() == "audio/opus") {
                format->mutable_container_audio()->set_container_audio_type(ContainerAudio::OGG_OPUS);
            } else if (generate.mime() == "audio/x-wav") {
                format->mutable_container_audio()->set_container_audio_type(ContainerAudio::WAV);
            } else if (generate.mime().StartsWith("audio/x-pcm;bit=16;rate=")) {
                TStringBuf mime = generate.mime();
                TStringBuf rate = mime.NextTok("rate=");
                RawAudio *raw = format->mutable_raw_audio();
                raw->set_audio_encoding(RawAudio::LINEAR16_PCM);
                raw->set_sample_rate_hertz(FromString(rate));
            }
        }

        std::shared_ptr<grpc::Channel> chan = grpc::CreateChannel(Config_.BackendUrl_, Config_.Insecure_ ? grpc::InsecureChannelCredentials() : grpc::SslCredentials({}));
        std::unique_ptr<Synthesizer::Stub> stub = Synthesizer::NewStub(chan);

        Callbacks_->OnRequestProcessingStarted(backendRequest.GetReqSeqNo());

        grpc::ClientContext grpcCxt;
        grpcCxt.AddMetadata("authorization", token);
        std::unique_ptr<grpc::ClientReader<UtteranceSynthesisResponse>> reader(stub->UtteranceSynthesis(&grpcCxt, request));
        UtteranceSynthesisResponse cloudResp;
        while (reader->Read(&cloudResp)) {
            requestMetrics.PushRate(cloudResp.ByteSizeLong(), "receiveddata", "ok");
            if (cloudResp.audio_chunk().data().empty()) {
                continue;
            }
            NProtobuf::TBackendResponse backendResponse;
            backendResponse.SetReqSeqNo(backendRequest.GetReqSeqNo());
            {
                auto& generateResponse = *backendResponse.MutableGenerateResponse();
                requestMetrics.PushRate(cloudResp.audio_chunk().data().size(), "receivedaudio", "ok");
                generateResponse.set_audiodata(cloudResp.audio_chunk().data());
                generateResponse.set_completed(false);
            }
            Callbacks_->OnBackendResponse(backendResponse, generate.mime());
        }
        grpc::Status status = reader->Finish();
        if (status.error_code() != grpc::StatusCode::OK) {
            requestMetrics.PushRate("grpcerror", ToString(status.error_code()));
            requestMetrics.SetError("grpcerror");
            Cancelled_ = true;
            rtLogActivation.Finish(/* ok = */ false);
            Callbacks_->OnAnyError(
                TStringBuilder()
                    << "code: " << status.error_code()
                    << " msg: " << status.error_message()
                    << " dtls: " << status.error_details() << "\n",
                false,
                backendRequest.GetReqSeqNo()
            );
        } else {
            requestMetrics.PushRate("request", "ok");
            NProtobuf::TBackendResponse backendResponse;
            backendResponse.SetReqSeqNo(backendRequest.GetReqSeqNo());
            {
                auto& generateResponse = *backendResponse.MutableGenerateResponse();
                generateResponse.set_completed(true);
            }
            rtLogActivation.Finish(/* ok = */ true);
            Callbacks_->OnBackendResponse(backendResponse, generate.mime());
        }
    } catch (...) {
        requestMetrics.PushRate("requestexception", "error");
        requestMetrics.SetError("requestexception");
        Cancelled_ = true;
        rtLogActivation.Finish(/* ok = */ false);
        Callbacks_->OnAnyError(CurrentExceptionMessage(), false, backendRequest.GetReqSeqNo());
    }
}

void TCloudSynth::Cancel() {
    Cancelled_ = true;
}
