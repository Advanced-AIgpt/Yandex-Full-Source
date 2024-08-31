#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/music_match/client/music_match_client.h>
#include <alice/cuttlefish/library/logging/dlog.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_writer.h>

#include <util/system/env.h>
#include <util/system/event.h>
#include <util/system/hostname.h>

namespace NDebug {
    static bool Enable = false;
}

namespace {
    constexpr TStringBuf TVM_SERVICE_TICKET_ENV_NAME = "TVM_SERVICE_TICKET";
    constexpr TStringBuf TVM_USER_TICKET_ENV_NAME = "TVM_USER_TICKET";

    void DebugOut(const TString& s) {
        if (NDebug::Enable) {
            Cout << TInstant::Now() << " " << s << Endl;
        }
    }
}

using namespace NAlice;
using namespace NAlice::NMusicMatch;

class TMusicMatchRequest: public TMusicMatchClient::TRequest {
public:
    TMusicMatchRequest(
        TMusicMatchClient& musicMatchClient,
        NAsio::TIOService& ioService,
        TManualEvent& event
    )
        : TMusicMatchClient::TRequest(musicMatchClient, ioService)
        , Event_(event)
    {
    }
    ~TMusicMatchRequest() override {
        DebugOut("~TMusicMatchRequest");
        Event_.Signal();
    }

    void SendMusic(
        const TString& filename,
        const TString& spotterFilename,
        size_t chunkSize,
        TDuration chunkDuration,
        NProtobuf::TContextLoadResponse&& contextLoadResponse,
        NProtobuf::TSessionContext&& sessionContext,
        NProtobuf::TInitRequest&& initRequest
    ) {
        Input_.Reset(new TFileInput(filename));
        if (spotterFilename) {
            InputSpotter_.Reset(new TFileInput(spotterFilename));
        }
        ChunkSize_ = chunkSize;
        Chunk_.resize(ChunkSize_);
        ChunkDuration_ = chunkDuration;
        AudioFormat_ = initRequest.GetAudioFormat();

        DebugOut("SendMusicMatchFlag");
        SendMusicMatchFlag();

        DebugOut("SendContextLoadResponseAndSessionConext");
        SendContextLoadResponseAndSessionContext(std::move(contextLoadResponse), std::move(sessionContext));

        DebugOut("SendInit");
        SendInitRequest(std::move(initRequest));

        DebugOut("Start async reading");
        NextAsyncRead();
    }

private:
    void OnInitResponse(NProtobuf::TInitResponse& initResponse) override {
        DebugOut(TStringBuilder() << "Got InitResponse: " << initResponse.ShortUtf8DebugString());

        RecvInitResponse_ = true;
        if (initResponse.GetIsOk()) {
            DebugOut(TStringBuilder() << "Send begin stream");
            SendBeginStream(AudioFormat_);
            if (InputSpotter_) {
                DebugOut(TStringBuilder() << "Send begin spotter");
                SendBeginSpotter();
            }

            SendNextChunk();
        } else {
            OnError(TStringBuilder() << "Not ok InitResponse: " << initResponse.GetErrorMessage());
        }
    }
    void OnStreamResponse(NProtobuf::TStreamResponse& streamResponse) override {
        DebugOut(TStringBuilder() << "Got StreamResponse: " << streamResponse.ShortUtf8DebugString());

        if (streamResponse.HasMusicResult()) {
            DebugOut(TStringBuilder() << "Got MusicResult: " << streamResponse.ShortUtf8DebugString());
            auto& musicResult = streamResponse.GetMusicResult();

            if (!musicResult.GetIsOk()) {
                DebugOut(TStringBuilder() << "Got MusicResult error: " << musicResult.GetErrorMessage());

                OnError(TStringBuilder() << "Not ok StreamResponse: " << musicResult.GetErrorMessage());
                return;
            }

            DebugOut(TStringBuilder() << "Got MusicResult json: " << musicResult.GetRawMusicResultJson());

            if (musicResult.GetIsFinish()) {
                DebugOut(TStringBuilder() << "Got MusicResult finish");

                RecvFinished_ = true;
                NeedNextRead_ = false;

                // We have got the result
                // There is no point in sending extra chunks
                StopSendingNextChunk();

                SendClose();

                return;
            }
        } else {
            DebugOut(TStringBuilder() << "Unexpected TStreamResponse type");
        }
    }

    bool NextAsyncRead() override {
        if (TMusicMatchClient::TRequest::NextAsyncRead()) {
            DebugOut("NextAsyncRead");
            return true;
        }
        return false;
    }
    void OnEndNextResponseProcessing() override {
        TMusicMatchClient::TRequest::OnEndNextResponseProcessing();
    }

    void OnTimeout() override {
        StopSendingNextChunk();
        TMusicMatchClient::TRequest::OnTimeout();
    }

    void OnError(const TString& s) override {
        // We have got the result
        // There is no point in sending extra chunks
        StopSendingNextChunk();

        NeedNextRead_ = false;
        RecvBadResponse_ = true;
        RecvFinished_ = true;

        TMusicMatchClient::TRequest::OnError(s);
        SendClose();
    }

    void SendNextChunk() {
        DebugOut("SendNextChunk");
        NextChunkTimer_.Reset();

        if (RecvBadResponse_) {
            DebugOut("Exta send next chunk after bad response");
            return;
        }
        if (LastChunkSended_) {
            DebugOut("Exta send next chunk after last chunk sended");
            return;
        }
        if (SendCloseConnection_) {
            DebugOut("Exta send next chunk after send close connection");
            return;
        }
        if (RecvFinished_) {
            DebugOut("Exta send next chunk after finished");
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
                DebugOut(TStringBuilder() << "Send spotter audio chunk: chunkSize=" << chunkSize);
            } else {
                DebugOut(TStringBuilder() << "Send audio chunk: chunkSize=" << chunkSize);
            }
            SendAudioChunk(&Chunk_[0], chunkSize);
        }
        if (chunkSize == ChunkSize_) {
            SetNextChunkTimer();
            return;
        }

        if (InputSpotter_) {
            DebugOut(TStringBuilder() << "Send end spotter");
            InputSpotter_.Reset();
            SendEndSpotter();
            if (chunkSize) {
                SetNextChunkTimer();
            } else {
                SendNextChunk();
            }
        } else {
            LastChunkSended_ = true;
            DebugOut(TStringBuilder() << "Send end stream");
            SendEndStream();
        }
    }

    void SetNextChunkTimer() {
        TAtomicSharedPtr<NAsio::TDeadlineTimer> timer(new NAsio::TDeadlineTimer(IOService_));
        TIntrusivePtr<TMusicMatchRequest> self(this);
        timer->AsyncWaitExpireAt(ChunkDuration_, [timer, self](const NAsio::TErrorCode& err, NAsio::IHandlingContext&){
            if (!err.Value()) {
                DebugOut("OnNextChunk timer");
                self->SendNextChunk();
            } else {
                DebugOut(TStringBuilder() << "OnNextChunk timer error: " << err.Text());
            }
        });
        NextChunkTimer_ = timer;
    }

    void SendClose() {
        if (!SendCloseConnection_) {
            if (!RecvFinished_) {
                DebugOut("Send CloseConnection before got finished");
            } else {
                DebugOut("Send CloseConnection");
            }

            SendCloseConnection_ = true;
            Stream_.WritesDone();
        }
    }

    void StopSendingNextChunk() {
        DebugOut("StopSendingNextChunk");
        if (NextChunkTimer_) {
            NextChunkTimer_->Cancel();
            NextChunkTimer_.Reset();
        }
    }

private:
    TManualEvent& Event_;
    THolder<TFileInput> Input_;
    THolder<TFileInput> InputSpotter_;
    size_t ChunkSize_;
    TVector<char> Chunk_;
    TDuration ChunkDuration_;
    TString AudioFormat_;
    TAtomicSharedPtr<NAsio::TDeadlineTimer> NextChunkTimer_;
    bool RecvInitResponse_ = false;
    bool RecvBadResponse_ = false;
    bool RecvFinished_ = false;
    bool SendCloseConnection_ = false;
    bool LastChunkSended_ = false;
};


int main(int argc, char *argv[]) {
    using namespace NLastGetopt;
    TOpts opts = NLastGetopt::TOpts::Default();

    opts.AddLongOption("debug").StoreResult(&NDebug::Enable).DefaultValue(true);

    bool appHostGraph;
    opts.AddLongOption("apphost-graph").StoreResult(&appHostGraph).DefaultValue(false);

    TMusicMatchClient::TConfig config;
    {
        opts.AddLongOption("host").StoreResult(&config.Host).DefaultValue("localhost");
        opts.AddLongOption("path").StoreResult(&config.Path).DefaultValue("/music_match");
        opts.AddLongOption("port").StoreResult(&config.Port).DefaultValue(10001);
        config.Timeout = TDuration::Seconds(100);

        // AppHost creates tvm ticket for the backend itself
        if (const auto& tvmServiceTicket = GetEnv(TString(TVM_SERVICE_TICKET_ENV_NAME)); !appHostGraph && tvmServiceTicket) {
            config.TvmTicket = tvmServiceTicket;
        }
    }

    TString requestId;
    opts.AddLongOption("request-id").StoreResult(&requestId).DefaultValue("");
    TString userIp;
    opts.AddLongOption("user-ip").StoreResult(&userIp).DefaultValue("");

    // Magic values from alice tests
    TString uuid;
    opts.AddLongOption("uuid").StoreResult(&uuid).DefaultValue("b36d2b1a-d2f4-11e9-919b-525400123456");
    TString appToken;
    opts.AddLongOption("app-token").StoreResult(&appToken).DefaultValue("069b6659-984b-4c5f-880e-aaedcfd84102");

    TString filename;
    opts.AddLongOption("file").StoreResult(&filename).DefaultValue("audio.opus");
    TString filenameSpotter; // Just for test legacy logic
    opts.AddLongOption("spotter-file").StoreResult(&filenameSpotter).DefaultValue("");
    TString mimeFormat;
    opts.AddLongOption("mime-format").StoreResult(&mimeFormat).DefaultValue("audio/opus");
    size_t chunkSize;
    opts.AddLongOption("chunk-size").StoreResult(&chunkSize).DefaultValue(200);
    size_t chunkDuration;
    opts.AddLongOption("chunk-duration").StoreResult(&chunkDuration).DefaultValue(200);

    TOptsParseResult res(&opts, argc, argv);
    NAsio::TExecutorsPool pool(1);

    NProtobuf::TContextLoadResponse contextLoadResponse;
    {
        if (const auto& tvmUserTicket = GetEnv(TString(TVM_USER_TICKET_ENV_NAME)); tvmUserTicket) {
            contextLoadResponse.SetUserTicket(tvmUserTicket);
        }
    }

    NProtobuf::TSessionContext sessionContext;
    {
        if (userIp) {
            auto* connectionInfo = sessionContext.MutableConnectionInfo();
            connectionInfo->SetIpAddress(userIp);
        }
        if (uuid) {
            auto* userInfo = sessionContext.MutableUserInfo();
            userInfo->SetUuid(uuid);
        }
        if (appToken) {
            sessionContext.SetAppToken(appToken);
        }
    }

    NProtobuf::TInitRequest initRequest;
    {
        if (requestId) {
            initRequest.SetRequestId(requestId);
        }

        if (mimeFormat) {
            initRequest.SetAudioFormat(mimeFormat);
        }

        {
            THttpHeaders headers;
            if (mimeFormat) {
                headers.AddHeader("Content-Type", mimeFormat);
            }

            TString headersString;
            {
                TStringOutput stringOutput(headersString);
                headers.OutTo(&stringOutput);
            }

            initRequest.SetHeaders(headersString);
        }
    }

    try {
        TManualEvent eventFinish;
        TMusicMatchClient musicMatchClient(config);
        {
            TMusicMatchRequest* musicMatch = new TMusicMatchRequest(musicMatchClient, pool.GetExecutor().GetIOService(), eventFinish);
            TMusicMatchRequest::TPtr musicMatchRequest(musicMatch);
            musicMatch->SendMusic(
                filename,
                filenameSpotter,
                chunkSize,
                TDuration::MilliSeconds(chunkDuration),
                std::move(contextLoadResponse),
                std::move(sessionContext),
                std::move(initRequest)
            );
        }
        eventFinish.WaitI();  // TODO: timeout?
        return 0;
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return 1;
    }
}
