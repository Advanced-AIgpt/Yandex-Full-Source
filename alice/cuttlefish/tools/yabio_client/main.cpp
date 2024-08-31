#include <alice/cuttlefish/library/yabio/client/yabio_client.h>
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
using namespace NAlice::NYabio;

class TYabioRequest: public TYabioClient::TRequest {
public:
    TYabioRequest(TYabioClient& yabioClient, NAsio::TIOService& ioService, TManualEvent& event, bool appHostGraph)
        : TYabioClient::TRequest(yabioClient, ioService)
        , Event_(event)
        , AppHostGraph_(appHostGraph)
    {
    }
    ~TYabioRequest() override {
        DebugOut("~TYabioRequest");
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
    }

private:
    void OnYabioResponse(NProtobuf::TResponse& response) override {
        if (response.HasInitResponse()) {
            DebugOut(TStringBuilder() << "got InitResponse: " << response.ShortUtf8DebugString());
            RecvInitResponse_ = true;
            auto initResponse = response.GetInitResponse();
            if (initResponse.GetresponseCode() == NProtobuf::RESPONSE_CODE_OK) {
                SendNextChunk();
            } else {
                // TODO: handle error (get error from response)
                OnError(TStringBuilder() << "not ok response: code=" << int(initResponse.GetresponseCode()));
            }
        } else if (response.HasAddDataResponse()) {
            // TODO: if got error close
            DebugOut(TStringBuilder() << "got AddDataResponse: " << response.ShortUtf8DebugString());
            auto addDataResponse = response.GetAddDataResponse();
            //bool closelastChunk = addDataResponse.HaslastChunk() && addDataResponse.GetlastChunk();
            //bool lastChunk = addDataResponse.HaslastChunk() && addDataResponse.GetlastChunk();
            NJson::TJsonValue j;
            //for (const auto& cr : addDataResponse.GettagToResult()) {
            //    j[""]
            //}
            for (const auto& cr : addDataResponse.GetclassificationResults()) {
                auto& lastResult = j["classification_results"].AppendValue(NJson::JSON_MAP);
                lastResult["tag"] = cr.tag();
                lastResult["classname"] = cr.classname();
            }
            for (const auto& cls : addDataResponse.classification()) {
                auto& lastResult = j["bioResult"].AppendValue(NJson::JSON_MAP);
                lastResult["tag"] = cls.tag();
                lastResult["classname"] = cls.classname();
                lastResult["confidence"] = cls.confidence();
            }
            //if (eou) {
            //    j["eou"] = true;
            //}
            //if (badSpotter) {
            //    j["spotter_confirm"] = false;
            //}
            /*TODO
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
            */
            if (j.IsDefined()) {
                Cout << WriteJson(j, false) << Endl;
            }
            /*
            if (eou || badSpotter) {
                SendClose();
                return;
            }
            */
        /*
        } else if (response.HasCloseConnection()) {
            DebugOut("got CloseConnection");
            RecvCloseConnection_ = true;
            if (!SendCloseConnection_) {
                SendClose();
            }
            // has close handshake (wait finish request)
            NeedNextRead_ = false;
        */
        } else {
            DebugOut("got ???");
        }
    }
    bool NextAsyncRead() override {
        if (TYabioClient::TRequest::NextAsyncRead()) {
            DebugOut("NextAsyncRead");
            return true;
        }
        return false;
    }
    void OnEndNextResponseProcessing() override {
        TYabioClient::TRequest::OnEndNextResponseProcessing();
    }
    void OnEndOfResponsesStream() override {
        DebugOut("EndOfResponsesStream");
    }
    void OnTimeout() override {
        StopSendingNextChunk();
        TYabioClient::TRequest::OnTimeout();
    }
    void OnError(const TString& s) override {
        StopSendingNextChunk();
        DebugOut(TStringBuilder() << "OnError: " << s);
        TYabioClient::TRequest::OnError(s);
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
            SendAudioChunk(&Chunk_[0], chunkSize);
            if (InputSpotter_) {
                //addData->SetSpotterChunk(true);
                DebugOut("send spotter chunk");
            } else {
                //addData->SetSpotterChunk(false);
                DebugOut("send chunk");
            }
            //TODO:send lastSpotterChunk = true
            /*
            NProtobuf::TRequest request;
            auto addData = request.MutableAddData();
            addData->SetaudioData(&Chunk_[0], chunkSize);
            addData->SetlastChunk(false);
            NAppHost::NClient::TInputDataChunk dataChunk;
            dataChunk.AddItem("INIT", ItemType_, request);
            Stream_.Write(std::move(dataChunk), false);
            */
        }
        if (chunkSize == ChunkSize_) {
            SetNextChunkTimer();
            return;
        }

        if (InputSpotter_) {
            DebugOut("spotter finished");
            InputSpotter_.Reset();
            if (chunkSize) {
                SetNextChunkTimer();
            } else {
                SendNextChunk();
            }
        } else {
            DebugOut("send EOS");
            SendEOS_ = true;
            SendEndStream();
            /*
            NProtobuf::TRequest request;
            auto addData = request.MutableAddData();
            addData->SetlastChunk(true);
            NAppHost::NClient::TInputDataChunk dataChunk;
            dataChunk.AddItem("INIT", ItemType_, request);
            Stream_.Write(std::move(dataChunk), false);
            */
        }
    }
    void SetNextChunkTimer() {
        TAtomicSharedPtr<NAsio::TDeadlineTimer> timer(new NAsio::TDeadlineTimer(IOService_));
        TIntrusivePtr<TYabioRequest> self(this);
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
            /*TODO
            DebugOut("send CloseConnection");
            SendCloseConnection_ = true;
            NProtobuf::TRequest request;
            request.MutableCloseConnection();
            NAppHost::NClient::TInputDataChunk dataChunk;
            dataChunk.AddItem("INIT", ItemType_, request);
            Stream_.Write(std::move(dataChunk), true);
            */
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
};


int main(int argc, char *argv[]) {
    using namespace NLastGetopt;
    TOpts opts = NLastGetopt::TOpts::Default();
    opts.AddLongOption("debug").StoreResult(&NDebug::Enable);
    TYabioClient::TConfig config;
    config.Host = "localhost";
    config.Port = 10001;
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
    TString mimeFormat = "audio/opus";
    opts.AddLongOption("mime-format").StoreResult(&mimeFormat);
    size_t chunkSize = 200;
    opts.AddLongOption("chunk-size").StoreResult(&chunkSize);
    size_t chunkDuration = 200;
    opts.AddLongOption("chunk-duration").StoreResult(&chunkDuration);
    TString method = "classify";
    opts.AddLongOption("method").StoreResult(&method);
    TString groupId = "test_group_id";
    opts.AddLongOption("group-id").StoreResult(&groupId);
    TVector<TString> tags;
    opts.AddLongOption("tag").AppendTo(&tags);
    TString userId = "test_user_id";
    opts.AddLongOption("user-id").StoreResult(&userId);

    NProtobuf::TInitRequest initRequest;
    TOptsParseResult res(&opts, argc, argv);
    NAsio::TExecutorsPool pool(1);
    if (requestId) {
        initRequest.SetMessageId(requestId);
    }
    initRequest.SethostName(config.Host);
    initRequest.SetsessionId(""); //TODO:
    initRequest.Setuuid(uuid);
    initRequest.Setmime(mimeFormat);
    if (method == "classify" || method == "c") {
        initRequest.Setmethod(NProtobuf::METHOD_CLASSIFY);
    } else if (method == "score" || method == "s") {
        initRequest.Setmethod(NProtobuf::METHOD_SCORE);
    } else {
        Cerr << "unknown yabio method='" << method << "'" << Endl;
        return 666;
    }
    initRequest.Setgroup_id(groupId);
    if (initRequest.method() == NProtobuf::METHOD_SCORE) {
        auto& context = *initRequest.Mutablecontext();
        context.set_group_id(groupId);
        auto& user = *context.add_users();
        user.set_user_id(userId);
        //TODO: user.add_voiceprints(...);
    }
    //initRequest.Setmodel_id(); //TODO
    initRequest.set_user_id(userId);
    //initRequest.Appendrequests_ids();  //TODO
    for (const auto& tag : tags) {
        initRequest.add_classification_tags(tag);
    }
    //initRequest.Mutablecontext(); //TODO
    initRequest.Setspotter(!filenameSpotter.empty());
    initRequest.SetclientHostname(FQDNHostName());
    //TODO: initRequest.Setexperiments("");
    try {
        TManualEvent eventFinish;
        TYabioClient yabioClient(config);
        {
            TYabioRequest* yabio = new TYabioRequest(yabioClient, pool.GetExecutor().GetIOService(), eventFinish, appHostGraph);
            TYabioRequest::TPtr yabioRequest(yabio);
            yabio->SendAudio(filenameSpotter, filename, mimeFormat, chunkSize, TDuration::MilliSeconds(chunkDuration), std::move(initRequest));
        }
        eventFinish.WaitI();  // TODO: timeout?
        return 0;
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return 1;
    }
}
