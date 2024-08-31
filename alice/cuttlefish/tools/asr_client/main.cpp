#include <alice/cuttlefish/library/asr/client/asr_client.h>
#include <alice/cuttlefish/library/logging/dlog.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_writer.h>

#include <util/system/event.h>
#include <util/system/hostname.h>

namespace NDebug {
    static bool Enable = false;
}

namespace {
    void DebugOut(const TString& s) {
        if (NDebug::Enable) {
            Cout << TInstant::Now() << " " << s << Endl;
        }
    }
}

using namespace NAlice;
using namespace NAlice::NAsr;

class TAsrRequest: public TAsrClient::TRequest {
public:
    TAsrRequest(TAsrClient& asrClient, NAsio::TIOService& ioService, TManualEvent& event, bool appHostGraph)
        : TAsrClient::TRequest(asrClient, ioService)
        , Event_(event)
        , AppHostGraph_(appHostGraph)
    {
    }
    ~TAsrRequest() override {
        DebugOut("~TAsrRequest");
        Event_.Signal();
    }

    void SendAudio(const TString filenameSpotter, const TString& filename, const TString& format, size_t chunkSize, TDuration chunkDuration, NProtobuf::TInitRequest&& initRequest) {
        (void)format;
        if (filenameSpotter) {
            InputSpotter_.Reset(new TFileInput(filenameSpotter));
        }
        Input_.Reset(new TFileInput(filename));
        ChunkSize_ = chunkSize;
        Chunk_.resize(ChunkSize_);
        ChunkDuration_ = chunkDuration;
        DebugOut("SendInit");
        SendInit(std::move(initRequest), AppHostGraph_);
        SendNextChunk();
    }

private:
    void OnAsrResponse(NProtobuf::TResponse& response) override {
        if (response.HasInitResponse()) {
            DebugOut(TStringBuilder() << "got InitResponse: " << response.ShortUtf8DebugString());
            RecvInitResponse_ = true;
            auto initResponse = response.GetInitResponse();
            if (initResponse.HasIsOk() && initResponse.GetIsOk()) {
                //SendNextChunk();
            } else {
                // TODO: handle error (get error from response)
                OnError(TStringBuilder() << "not ok response: " << initResponse.GetErrMsg());
            }
        } else if (response.HasAddDataResponse()) {
            // TODO: if got error close
            DebugOut(TStringBuilder() << "got AddDataResponse: " << response.ShortUtf8DebugString());
            auto addDataResponse = response.GetAddDataResponse();
            bool eou = addDataResponse.HasResponseStatus() && addDataResponse.GetResponseStatus() == NProtobuf::EndOfUtterance;
            bool badSpotter = addDataResponse.HasResponseStatus() && addDataResponse.GetResponseStatus() == NProtobuf::ValidationFailed;
            NJson::TJsonValue j;
            if (eou) {
                j["eou"] = true;
            }
            if (badSpotter) {
                j["spotter_confirm"] = false;
            }
            if (addDataResponse.HasRecognition()) {
                const auto& recognition = addDataResponse.GetRecognition();
                auto& hypos = recognition.GetHypos();
                if (hypos.size()) {
                    if (hypos[0].HasNormalized()) {
                        j["normalized"] = hypos[0].GetNormalized();
                    }
                    if (hypos[0].GetWords().size()) {
                        NJson::TJsonValue& words = j["words"];
                        words = NJson::JSON_ARRAY;
                        for (auto& word : hypos[0].GetWords()) {
                            words.AppendValue(word);
                        }
                    }
                }
            }
            if (j.IsDefined()) {
                Cout << WriteJson(j, false) << Endl;
            }
            if (eou || badSpotter) {
                SendClose();
                return;
            }
        } else if (response.HasCloseConnection()) {
            DebugOut("got CloseConnection");
            RecvCloseConnection_ = true;
            if (!SendCloseConnection_) {
                SendClose();
            }
            // has close handshake (wait finish request)
            NeedNextRead_ = false;
        } else {
            DebugOut("got ???");
        }
    }
    bool NextAsyncRead() override {
        if (TAsrClient::TRequest::NextAsyncRead()) {
            DebugOut("NextAsyncRead");
            return true;
        }
        return false;
    }
    void OnEndNextResponseProcessing() override {
        TAsrClient::TRequest::OnEndNextResponseProcessing();
    }
    void OnEndOfResponsesStream() override {
        DebugOut("EndOfResponsesStream");
    }
    void OnTimeout() override {
        StopSendingNextChunk();
        TAsrClient::TRequest::OnTimeout();
    }
    void OnError(const TString& s) override {
        StopSendingNextChunk();
        DebugOut(TStringBuilder() << "OnError: " << s);
        TAsrClient::TRequest::OnError(s);
        // TODO: close stream if need
    }
    void SendNextChunk() {
        DebugOut("SendNextChunk");
        NextChunkTimer_.Reset();
        if (SendEOS_ || SendCloseConnection_) {
            Cerr << "exta send next chunk";
            return;
        }

        size_t chunkSize;;
        if (InputSpotter_) {
            chunkSize = InputSpotter_->Load(&Chunk_[0], ChunkSize_);
        } else {
            chunkSize = Input_->Load(&Chunk_[0], ChunkSize_);
        }
        if (chunkSize) {
            if (InputSpotter_) {
                DebugOut("send spotter chunk");
            } else {
                DebugOut("send chunk");
            }
            SendAudioChunk(&Chunk_[0], chunkSize);
        }
        if (chunkSize == ChunkSize_) {
            SetNextChunkTimer();
            return;
        }

        if (InputSpotter_) {
            DebugOut("spotter finished");
            InputSpotter_.Reset();
            SendEndSpotter();
            if (chunkSize) {
                SetNextChunkTimer();
            } else {
                SendNextChunk();
            }
        } else {
            DebugOut("send EOS");
            SendEOS_ = true;
            SendEndStream();
        }
    }
    void SetNextChunkTimer() {
        TAtomicSharedPtr<NAsio::TDeadlineTimer> timer(new NAsio::TDeadlineTimer(IOService_));
        TIntrusivePtr<TAsrRequest> self(this);
        timer->AsyncWaitExpireAt(ChunkDuration_, [timer, self](const NAsio::TErrorCode& err, NAsio::IHandlingContext&){
            DebugOut("OnNextChunk timer");
            if (!err.Value()) {
                self->SendNextChunk();
            }
            (void)timer;
        });
        NextChunkTimer_ = timer;
    }
    void SendClose() {
        if (!SendCloseConnection_) {
            DebugOut("send CloseConnection");
            SendCloseConnection_ = true;
            NProtobuf::TRequest request;
            request.MutableCloseConnection();
            NAppHost::NClient::TInputDataChunk dataChunk;
            dataChunk.AddItem("INIT", ItemType_, request);
            Stream_.Write(std::move(dataChunk), true);
        }
    }
    void StopSendingNextChunk() {
        DebugOut("StopSendingNextChunk()");
        if (NextChunkTimer_) {
            NextChunkTimer_->Cancel();
            NextChunkTimer_.Reset();
        }
    }

private:
    TManualEvent& Event_;
    bool AppHostGraph_;
    THolder<TFileInput> InputSpotter_;
    THolder<TFileInput> Input_;
    size_t ChunkSize_;
    TVector<char> Chunk_;
    TDuration ChunkDuration_;
    TAtomicSharedPtr<NAsio::TDeadlineTimer> NextChunkTimer_;
    bool RecvInitResponse_ = false;
    bool SendEOS_ = false;
    bool SendCloseConnection_ = false;
    bool RecvCloseConnection_ = false;
};


int main(int argc, char *argv[]) {
    using namespace NLastGetopt;
    TOpts opts = NLastGetopt::TOpts::Default();
    opts.AddLongOption("debug").StoreResult(&NDebug::Enable);
    TAsrClient::TConfig config;
    config.Host = "localhost";
    config.Port = 10501;
    config.Timeout = TDuration::Seconds(100);
    opts.AddLongOption("host").StoreResult(&config.Host);
    opts.AddLongOption("path").StoreResult(&config.Path);
    opts.AddLongOption("port").StoreResult(&config.Port);
    bool appHostGraph = false;  // use direct request to backend instead apphost graph
    opts.AddLongOption("apphost-graph").StoreResult(&appHostGraph);
    TString requestId;
    opts.AddLongOption("request-id").StoreResult(&requestId);
    TString appId;
    opts.AddLongOption("app-id").StoreResult(&appId);
    TString uuid = "ffffffffffffffffffffffffffffffff";
    opts.AddLongOption("uuid").StoreResult(&uuid);
    TString filename = "audio.opus";
    opts.AddLongOption("file").StoreResult(&filename);
    TString filenameSpotter = "";
    opts.AddLongOption("spotter-file").StoreResult(&filenameSpotter);
    TString spotterPhrase = "";
    opts.AddLongOption("spotter-phrase").StoreResult(&spotterPhrase);
    TString mimeFormat = "audio/opus";
    opts.AddLongOption("mime-format").StoreResult(&mimeFormat);
    size_t chunkSize = 200;
    opts.AddLongOption("chunk-size").StoreResult(&chunkSize);
    size_t chunkDuration = 200;
    opts.AddLongOption("chunk-duration").StoreResult(&chunkDuration);

    NProtobuf::TInitRequest initRequest;
    TString topic = "unknown-topic";
    opts.AddLongOption("topic").StoreResult(&topic);
    TString language = "ru";
    opts.AddLongOption("language").StoreResult(&language);
    bool punctuation = true;
    opts.AddLongOption("punctuation").StoreResult(&punctuation);
    bool manualPunctuation = false;
    opts.AddLongOption("manual-punctuation").StoreResult(&manualPunctuation);
    bool normalization = true;
    opts.AddLongOption("normalization").StoreResult(&normalization);
    bool capitalization = true;
    opts.AddLongOption("capitalization").StoreResult(&capitalization);
    bool antimat = true;
    opts.AddLongOption("antimat").StoreResult(&antimat);
    // asr protobuf v2 not support spotter front/back
    /*
    unsigned spotterBack = 2000;
    opts.AddLongOption("spotter-back").StoreResult(&spotterBack);
    unsigned requestFront = 1000;
    opts.AddLongOption("request-front").StoreResult(&requestFront);
    */

    TOptsParseResult res(&opts, argc, argv);
    NAsio::TExecutorsPool pool(1);
    if (requestId) {
        initRequest.SetRequestId(requestId);
    }
    initRequest.SetAppId(appId);
    initRequest.SetUuid(uuid);
    initRequest.SetHostName(config.Host);
    initRequest.SetClientHostname(FQDNHostName());
    initRequest.SetDevice("cuttlefish asr_client");
    initRequest.SetMime(mimeFormat);
    initRequest.SetTopic(topic);
    initRequest.SetLang(language);
    initRequest.SetEouMode(NProtobuf::SingleUtterance);
    auto& recognitionOptions = *initRequest.MutableRecognitionOptions();
    if (spotterPhrase && filenameSpotter) {
        initRequest.SetHasSpotterPart(true);
        recognitionOptions.SetSpotterPhrase(spotterPhrase);
    } else {
        initRequest.SetHasSpotterPart(false);
    }
    recognitionOptions.SetPunctuation(punctuation);
    recognitionOptions.SetNormalization(normalization);
    recognitionOptions.SetCapitalization(capitalization);
    recognitionOptions.SetAntimat(antimat);
    recognitionOptions.SetManualPunctuation(manualPunctuation);
    // asr protobuf v2 not support spotter front/back
    //recognitionOptions.SetSpotterBack(spotterBack);
    //recognitionOptions.SetRequestFront(requestFront);

    try {
        TManualEvent eventFinish;
        TAsrClient asrClient(config);
        {
            TAsrRequest* asr = new TAsrRequest(asrClient, pool.GetExecutor().GetIOService(), eventFinish, appHostGraph);
            TAsrRequest::TPtr asrRequest(asr);
            asr->SendAudio(filenameSpotter, filename, mimeFormat, chunkSize, TDuration::MilliSeconds(chunkDuration), std::move(initRequest));
        }
        eventFinish.WaitI();  // TODO: timeout?
        return 0;
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return 1;
    }
}
