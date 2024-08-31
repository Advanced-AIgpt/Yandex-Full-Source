#include <alice/cuttlefish/library/logging/dlog.h>
#include <alice/cuttlefish/library/tts/backend/client/tts_client.h>

#include <voicetech/library/proto_api/ttsbackend.pb.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/system/event.h>
#include <util/system/hostname.h>

using namespace NAlice;

class TTtsRequest: public TTtsClient::TRequest {
public:
    TTtsRequest(
        TTtsClient& ttsClient,
        NAsio::TIOService& ioService,
        TManualEvent& event
    )
        : TTtsClient::TRequest(ttsClient, ioService)
        , Event_(event)
    {
    }
    ~TTtsRequest() override {
        DLOG("~TTtsRequest");
        Event_.Signal();
    }

    void SendRequests(
        const TString& fileNamePrefix,
        const TString& fileNameSuffix,
        NTts::TBackendRequest&& backendRequestBase,
        TVector<TString>&& textsToGenerate,
        TDuration durationBetweenRequests
    ) {
        BackendRequestBase_ = backendRequestBase;
        TextToGeneratePtr_ = 0;
        TextsToGenerate_ = textsToGenerate;
        for (size_t i = 0; i < TextsToGenerate_.size(); ++i) {
            OutputFiles_.push_back(MakeHolder<TFileOutput>(TStringBuilder() << fileNamePrefix << i << fileNameSuffix));
        }
        DurationBetweenRequests_ = durationBetweenRequests;

        SendNextRequest();
        NextAsyncRead();
    }

private:
    void OnProtocolAudio(NAliceProtocol::TAudio& audio) override {
        TMaybe<ui32> reqSeqNo = Nothing();
        if (audio.HasTtsBackendResponse()) {
            const NTts::TBackendResponse& ttsBackendResponse = audio.GetTtsBackendResponse();
            reqSeqNo = ttsBackendResponse.GetReqSeqNo();
            DLOG("Got tts meta info: " << ttsBackendResponse.ShortUtf8DebugString());

            if (ttsBackendResponse.HasGenerateResponse()) {
                auto generateResponse = ttsBackendResponse.GetGenerateResponse();
                if (generateResponse.has_responsecode() && generateResponse.responsecode() != 200) {
                    // handle error (get error from response)
                    OnError(TStringBuilder() << "not ok responseCode=" << int(generateResponse.responsecode()) << ":" << generateResponse.message());
                }
            }
        } else {
            OnError("ReqSeqNo not provided");
        }

        // Even after error DLOG all other fields
        if (audio.HasChunk()) {
            auto& chunk = audio.GetChunk();
            if (chunk.HasData() && chunk.GetData().size()) {
                DLOG("Got audio chunk: size=" << chunk.GetData().size());
                if (reqSeqNo.Defined()) {
                    if (*reqSeqNo < OutputFiles_.size()) {
                        OutputFiles_[*reqSeqNo]->Write(chunk.GetData().data(), chunk.GetData().size());
                    } else {
                        DLOG("ReqSeqNo " << *reqSeqNo << " is too big, only reqSeqNo less " << OutputFiles_.size() << " expected");
                    }
                } else {
                    DLOG("Audio chunk with unknown reqSeqNo ignored");
                }
            }
        } else {
            DLOG("Got: " << audio.ShortUtf8DebugString());
        }
    }
    bool NextAsyncRead() override {
        if (TTtsClient::TRequest::NextAsyncRead()) {
            DLOG("NextAsyncRead");
            return true;
        }

        return false;
    }
    void OnEndNextResponseProcessing() override {
        TTtsClient::TRequest::OnEndNextResponseProcessing();
    }
    void OnEndOfResponsesStream() override {
        DLOG("EndOfResponsesStream");
    }
    void OnTimeout() override {
        StopSendingNextRequest();
        TTtsClient::TRequest::OnTimeout();
    }
    void OnError(const TString& s) override {
        TTtsClient::TRequest::OnError(s);
    }

    void SendNextRequest() {
        DLOG("SendNextRequest");
        if (TextToGeneratePtr_ == TextsToGenerate_.size()) {
            if (TextsToGenerate_.empty()) {
                DLOG("Cancel (no requests)");
                Cancel();
            } else {
                DLOG("SendEndOfStream");
                SendEndOfStream();
            }
        } else {
            auto& currentText = TextsToGenerate_[TextToGeneratePtr_];
            ::NTts::TBackendRequest currentRequest = BackendRequestBase_;
            currentRequest.MutableGenerate()->set_text(currentText);
            currentRequest.SetReqSeqNo(TextToGeneratePtr_++);

            SendBackendRequest(std::move(currentRequest));
            SetNextRequestTimer();
        }
    }

    void SetNextRequestTimer() {
        TAtomicSharedPtr<NAsio::TDeadlineTimer> timer(new NAsio::TDeadlineTimer(IOService_));
        TIntrusivePtr<TTtsRequest> self(this);
        timer->AsyncWaitExpireAt(DurationBetweenRequests_, [timer, self](const NAsio::TErrorCode& err, NAsio::IHandlingContext&){
            if (!err.Value()) {
                DLOG("OnNextRequest timer");
                self->SendNextRequest();
            } else {
                DLOG(TStringBuilder() << "OnNextRequest timer error: " << err.Text());
            }
        });
        NextRequestTimer_ = timer;
    }

    void StopSendingNextRequest() {
        DLOG("StopSendingNextRequest");
        if (NextRequestTimer_) {
            NextRequestTimer_->Cancel();
            NextRequestTimer_.Reset();
        }
    }

private:
    TManualEvent& Event_;

    ::NTts::TBackendRequest BackendRequestBase_;
    size_t TextToGeneratePtr_ = 0;
    TVector<TString> TextsToGenerate_;
    TVector<THolder<TFileOutput>> OutputFiles_;
    TDuration DurationBetweenRequests_ = TDuration::Zero();
    TAtomicSharedPtr<NAsio::TDeadlineTimer> NextRequestTimer_;
};


int main(int argc, char *argv[]) {
    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();

    TTtsClient::TConfig config;
    {
        opts.AddLongOption("host").StoreResult(&config.Host).DefaultValue("localhost");
        opts.AddLongOption("port").StoreResult(&config.Port).DefaultValue(10001);
        opts.AddLongOption("path").StoreResult(&config.Path).DefaultValue("/tts");
    }

    TString requestId;
    opts.AddLongOption("request-id").StoreResult(&requestId);

    TString fileNamePrefix;
    opts.AddLongOption("file-name-prefix").StoreResult(&fileNamePrefix).DefaultValue("audio_");
    TString fileNameSuffix;
    opts.AddLongOption("file-name-suffix").StoreResult(&fileNameSuffix).DefaultValue(".opus");

    TVector<TString> textsToGenerate;
    opts
        .AddLongOption("text", "(repeatable) add one more text to generate")
        .Handler1T<TString>([&textsToGenerate](const TString& text) {
            textsToGenerate.push_back(text);
        })
    ;

    TString mimeFormat;
    opts.AddLongOption("mime-format").StoreResult(&mimeFormat).DefaultValue("audio/opus");
    TString lang;
    opts.AddLongOption("lang").StoreResult(&lang).DefaultValue("ru");
    double speed;
    opts.AddLongOption("speed").StoreResult(&speed).DefaultValue(1.0);
    bool chunked;
    opts.AddLongOption("chunked").StoreResult(&chunked).DefaultValue(false);

    TDuration durationBetweenRequests;
    opts.AddLongOption("duration-between-requests").StoreResult(&durationBetweenRequests).DefaultValue("200ms");

    NLastGetopt::TOptsParseResult res(&opts, argc, argv);

    ::NTts::TBackendRequest backendRequestBase;
    {
        TTS::Generate& generateRequest = *backendRequestBase.MutableGenerate();
        if (requestId) {
            generateRequest.set_message_id(requestId);
        }
        generateRequest.set_lang(lang);
        generateRequest.set_mime(mimeFormat);
        generateRequest.set_speed(speed);
        generateRequest.set_clienthostname(FQDNHostName());
        generateRequest.set_chunked(chunked);
    }

    NAsio::TExecutorsPool pool(1);
    try {
        TManualEvent eventFinish;
        TTtsClient ttsClient(config);
        {
            TTtsRequest::TPtr ttsRequest = new TTtsRequest(
                ttsClient,
                pool.GetExecutor().GetIOService(),
                eventFinish
            );
            dynamic_cast<TTtsRequest*>(ttsRequest.Get())->SendRequests(
                fileNamePrefix,
                fileNameSuffix,
                std::move(backendRequestBase),
                std::move(textsToGenerate),
                durationBetweenRequests
            );
        }
        eventFinish.WaitI();
        return 0;
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return 1;
    }
}
