#include <alice/cuttlefish/library/api/handlers.h>
#include <alice/cuttlefish/library/cuttlefish_client/cuttlefish_client.h>
#include <alice/cuttlefish/library/logging/dlog.h>
#include <alice/cuttlefish/library/protos/session.pb.h>
#include <voicetech/library/messages/message.h>
#include <voicetech/library/messages/message_to_wsevent.h>
#include <voicetech/library/proto_api/ttsbackend.pb.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/protobuf/json/json2proto.h>

#include <util/system/event.h>
#include <util/system/hostname.h>

using namespace NAlice;
using namespace NAlice::NCuttlefish;
using namespace NAliceProtocol;

namespace NDebug {
    static bool Enable = false;
}

namespace {
    void DebugOut(const TString& s) {
        if (NDebug::Enable) {
            Cout << TInstant::Now() << " " << s << Endl;
        }
    }
    TString ASR_RECOGNIZE = R"({
        "event": {
            "header": {
                "namespace": "ASR",
                "name": "Recognize",
                "streamId": 1
            },
            "payload": {
                "lang": "ru-RU",
                "topic": "quasar-general-gpu",
                "application": "cuttlefish_client",
                "format": "audio/opus",
                "key": "developers-simple-key",
                "punctuation": true,
                "advancedASROptions": {
                    "partial_results": true
                }
            }
        }
    })";
    TString VINS_VOICE_INPUT = R"({
        "event": {
            "header": {
                "namespace": "Vins",
                "name": "VoiceInput",
                "streamId": 1
            },
            "payload": {
                "header": {
                    "request_id": ""
                },
                "lang": "ru-RU",
                "topic": "quasar-general-gpu"
                "request": {
                    "experiments": [ "enable_e2e_eou" ],
                    "event": {
                        "type": "voice_input"
                    },
                    "voice_session": true
                },
                "realtime_streamer": {
                    "opus": {
                        "enabled": false
                    }
                },
                "reset_session": true,
            }
        }
    })";
    TString TTS_GENERATE = R"({
        "event": {
            "header": {
                "namespace": "TTS",
                "name": "Generate"
            },
            "payload": {
                "text": "1 2 3 4",
                "voice": "shitova.gpu",
                "lang": "ru-RU",
                "disable_tts_fallback": true,
                "platform": "test",
                "application": "test",
                "format": "Opus",
                "quality": "UltraHigh",
                "disable_cache": true
            }
        }
    })";
    TString ASR_RESPONSE_INIT_OK = R"({
        "InitResponse": {
            "IsOk": true,
            "Hostname": "client_cuttelfish_hostname",
            "Topic": "client_cuttelfish_topic",
            "TopicVersion": "client_cuttelfish_topic_version",
            "ServerVersion": "client_cuttelfish_server_version"
        }
    })";
    TString ASR_RESPONSE_ADD_DATA_OK = R"({
        "AddDataResponse": {
            "IsOk": true,
            "ResponseStatus": "Active",
            "ValidationInvoked": false,
            "MessagesCount": 1,
            "DurationProcessedAudio": 100
        }
    })";

    NAliceProtocol::TWsEvent StreamControl(i64 streamId, int action, const TString messageId = "ffffffff-ffff-ffff-ffff-ffffffffeeee") {
        //int action = 0;  // ACTION_CLOSE
        //int action = 2;  // ACTION_SPOTTER_END
        NAliceProtocol::TWsEvent wsEvent;
        auto& evHeader = *wsEvent.MutableHeader();
        evHeader.SetNamespace(TEventHeader::STREAM_CONTROL);
        evHeader.SetMessageId(messageId);
        evHeader.SetStreamId(streamId);
        evHeader.SetAction(action);
        evHeader.SetReason(0);
        TStringStream raw;
        raw << "{\"streamcontrol\":{\"messageId\":\"" << messageId << "\",\"reason\":0,\"action\":" << action << ",\"streamId\":" << streamId << "}}";
        wsEvent.SetText(raw.Str());
        return std::move(wsEvent);
    }
}

class TCuttlefishRequest: public TCuttlefishClient::TRequest {
public:
    TCuttlefishRequest(TCuttlefishClient& client, NAsio::TIOService& ioService, TManualEvent& event, bool appHostGraph)
        : TCuttlefishClient::TRequest(client, ioService)
        , Event_(event)
        , AppHostGraph_(appHostGraph)
    {
        (void)AppHostGraph_; //TODO:
    }
    ~TCuttlefishRequest() override {
        DebugOut("~TCuttlefishRequest");
        Event_.Signal();
    }

    void SendAudio(const TString filenameSpotter, const TString& filename, const TString& format, size_t chunkSize, TDuration chunkDuration, NAliceProtocol::TWsEvent& wsEvent) {
        (void)format;
        if (filenameSpotter) {
            InputSpotter_.Reset(new TFileInput(filenameSpotter));
        }
        Input_.Reset(new TFileInput(filename));
        ChunkSize_ = chunkSize;
        Chunk_.resize(ChunkSize_);
        ChunkDuration_ = chunkDuration;
        DebugOut(TStringBuilder() << "send wsEvent: " << wsEvent.ShortUtf8DebugString());
        //TODO: parse wsEventText to Header?!
        StreamId = wsEvent.GetHeader().GetStreamId();
        SendWsEvent(wsEvent); //TODO:?, AppHostGraph_);
        NextAsyncRead();
        SendNextChunk();
    }

    void SendEvent(NAliceProtocol::TWsEvent& wsEvent) {
        DebugOut(TStringBuilder() << "send wsEvent: " << wsEvent.ShortUtf8DebugString());
        //TODO: parse wsEventText to Header?!
        StreamId = wsEvent.GetHeader().GetStreamId();
        SendWsEvent(wsEvent); //TODO:, AppHostGraph_);
        NextAsyncRead();
    }

    void SendAsrResponse(NAlice::NAsr::NProtobuf::TResponse& response) {
        DebugOut(TStringBuilder() << "send AsrResponse: " << response.ShortUtf8DebugString());
        TCuttlefishClient::TRequest::SendAsrResponse(response, true);
        NextAsyncRead();
    }

private:
    void OnCuttlefishResponse(const NAliceProtocol::TAudio& audio) override {
        if (audio.HasChunk()) {
            if (audio.GetChunk().HasData()) {
                DebugOut(TStringBuilder() << "got audio chunk: size=" << audio.GetChunk().GetData().Size());
            } else {
                DebugOut(TStringBuilder() << "got audio chunk without data");
            }
        } else {
            DebugOut(TStringBuilder() << "got audio: " << audio.ShortUtf8DebugString());
        }
    }
    void OnCuttlefishResponse(const NAliceProtocol::TDirective& directive) override {
        DebugOut(TStringBuilder() << "got directive: " << directive.ShortUtf8DebugString());
        // json dump for directive
        NJson::TJsonValue json;
        using namespace NProtobufJson;
        TProto2JsonConfig cfg;
        cfg.SetMissingSingleKeyMode(TProto2JsonConfig::MissingKeyDefault);
        Proto2Json(directive, json, cfg);
        Cout << WriteJson(json, false) << Endl;
    }
    void OnCuttlefishResponse(const NAliceProtocol::TWsEvent& wsEvent) override {
        DebugOut(TStringBuilder() << "got wsEvent: " << wsEvent.ShortUtf8DebugString());
        if (wsEvent.HasText()) {
            Cout << wsEvent.GetText() << Endl;
        }
        //if (ws)
    }
    void OnCuttlefishResponse(const NAlice::NTts::NProtobuf::TBackendRequest& ttsRequest) override {
        DebugOut(TStringBuilder() << "got TtsRequest: " << ttsRequest.ShortUtf8DebugString());
        if (ttsRequest.HasGenerate()) {
            Cout << ttsRequest.GetGenerate().text() << Endl;
        }
    }
    bool NextAsyncRead() override {
        if (TCuttlefishClient::TRequest::NextAsyncRead()) {
            DebugOut("NextAsyncRead");
            return true;
        }
        return false;
    }
    void OnEndNextResponseProcessing() override {
        TCuttlefishClient::TRequest::OnEndNextResponseProcessing();
    }
    void OnEndOfResponsesStream() override {
        DebugOut("EndOfResponsesStream");
    }
    void OnTimeout() override {
        StopSendingNextChunk();
        TCuttlefishClient::TRequest::OnTimeout();
    }
    void OnError(const TString& s) override {
        StopSendingNextChunk();
        DebugOut(TStringBuilder() << "OnError: " << s);
        TCuttlefishClient::TRequest::OnError(s);
    }
    void SendNextChunk() {
        DebugOut("SendNextChunk");
        NextChunkTimer_.Reset();
        if (SendEOS_) {
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
            NAliceProtocol::TWsEvent event;
            // add stream id
            TString chunk;
            chunk.reserve(4 + chunkSize + 1);
            ui32 netStreamId = InetToHost(static_cast<const ui32>(StreamId));
            chunk.append(reinterpret_cast<const char*>(&netStreamId), sizeof(netStreamId));
            chunk.append(&Chunk_[0], chunkSize);
            //Log(TStringBuilder() << "send chunk stream_id=" << streamId << " size=" << needSend);
            event.SetBinary(chunk);
            if (InputSpotter_) {
                DebugOut(TStringBuilder() << "send spotter chunk size=" << chunkSize);
            } else {
                DebugOut(TStringBuilder() << "send chunk size=" << chunkSize);
            }
            SendWsEvent(event);
        }
        if (chunkSize == ChunkSize_) {
            SetNextChunkTimer();
            return;
        }

        if (InputSpotter_) {
            DebugOut("spotter finished");
            InputSpotter_.Reset();
            // send streamcontrol end of spotter
            auto endOfSpotter = StreamControl(StreamId, 2, "ffffffff-ffff-ffff-ffff-ffffffffcccc");
            DebugOut(TStringBuilder() << "send wsEvent: " << endOfSpotter.ShortUtf8DebugString());
            SendWsEvent(endOfSpotter);
            if (chunkSize) {
                SetNextChunkTimer();
            } else {
                SendNextChunk();
            }
        } else {
            SendEOS_ = true;
            // send streamcontrol end of stream, use write final flag
            auto eos = StreamControl(StreamId, 0);
            DebugOut(TStringBuilder() << "send last wsEvent: " << eos.ShortUtf8DebugString());
            SendWsEvent(eos, true);
        }
    }
    void SetNextChunkTimer() {
        TAtomicSharedPtr<NAsio::TDeadlineTimer> timer(new NAsio::TDeadlineTimer(IOService_));
        TIntrusivePtr<TCuttlefishRequest> self(this);
        timer->AsyncWaitExpireAt(ChunkDuration_, [timer, self](const NAsio::TErrorCode& err, NAsio::IHandlingContext&){
            DebugOut("OnNextChunk timer");
            if (!err.Value()) {
                self->SendNextChunk();
            }
            (void)timer;
        });
        NextChunkTimer_ = timer;
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
    ui32 StreamId = 1;
    bool SendEOS_ = false;
};


int main(int argc, char *argv[]) {
    using namespace NLastGetopt;
    TOpts opts = NLastGetopt::TOpts::Default();
    opts.AddLongOption("debug").StoreResult(&NDebug::Enable);
    TCuttlefishClient::TConfig config;
    config.Host = "localhost";
    config.Port = 4001;
    config.Path = "";
    config.Timeout = TDuration::Seconds(100);
    opts.AddLongOption("host").StoreResult(&config.Host);
    opts.AddLongOption("path").StoreResult(&config.Path);
    opts.AddLongOption("port").StoreResult(&config.Port);
    bool appHostGraph = false;  // use direct request to backend instead apphost graph
    opts.AddLongOption("apphost-graph").StoreResult(&appHostGraph);
    //TString requestId;
    //opts.AddLongOption("request-id").StoreResult(&requestId);
    //TString appId;
    //opts.AddLongOption("app-id").StoreResult(&appId);
    //TString uuid = "ffffffffffffffffffffffffffffffff";
    //opts.AddLongOption("uuid").StoreResult(&uuid);
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
    TString method;
    opts.AddLongOption("method").StoreResult(&method);
    TString jsonEvent;
    opts.AddLongOption("json-event").StoreResult(&jsonEvent);
    TString error;
    opts.AddLongOption("error").StoreResult(&error);
/*
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
*/
    TOptsParseResult res(&opts, argc, argv);
    NAsio::TExecutorsPool pool(1);
    NAliceProtocol::TWsEvent wsEvent;
//    wsMessage
/*

    if (requestId) {
        initRequest.SetRequestId(requestId);
    }
    initRequest.SetAppId(appId);
    initRequest.SetUuid(uuid);
    initRequest.SetHostName(config.Host);
    initRequest.SetClientHostname(FQDNHostName());
    initRequest.SetDevice("cuttlefish_client");
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
*/

    try {
        if (method == "asr.recognize") {
            if (!config.Path) {
                config.Path = SERVICE_HANDLE_STREAM_RAW_TO_PROTOBUF;
            }
            if (!jsonEvent) {
                jsonEvent = ASR_RECOGNIZE;
            }
        } else if (method == "vins.voiceinput") {
            if (!config.Path) {
                config.Path = SERVICE_HANDLE_STREAM_RAW_TO_PROTOBUF;
            }
            if (!jsonEvent) {
                jsonEvent = VINS_VOICE_INPUT;
            }
        } else if (method == "asr.response") {
            if (!config.Path) {

                config.Path = SERVICE_HANDLE_STREAM_PROTOBUF_TO_RAW;
            }
            if (!jsonEvent) {
                jsonEvent = ASR_RESPONSE_INIT_OK;
            }
        } else if (method == "tts.generate") {
            if (!config.Path) {
                config.Path = SERVICE_HANDLE_STREAM_RAW_TO_PROTOBUF;
            }
            if (!jsonEvent) {
                jsonEvent = TTS_GENERATE;
            }
        } else {
            throw yexception() << "unknown method=\"" << method << "\"";
        }
        using namespace NVoicetech::NUniproxy2;
        NJson::TJsonValue jsonVal;
        NJson::ReadJsonTree(jsonEvent, &jsonVal, true);
        TManualEvent eventFinish;
        TCuttlefishClient client(config);
        {
            TCuttlefishRequest* request = new TCuttlefishRequest(client, pool.GetExecutor().GetIOService(), eventFinish, appHostGraph);
            TCuttlefishRequest::TPtr tmpHolder(request);
            if (method == "asr.recognize" || method == "vins.voiceinput") {
                jsonVal.SetValueByPath("event.header.messageId", "ffffffff-ffff-ffff-ffff-ffffffff0000");
                if (filenameSpotter) {
                    jsonVal.SetValueByPath("event.payload.enable_spotter_validation", true);
                    if (!spotterPhrase) {
                        spotterPhrase = "алиса";
                    }
                    jsonVal.SetValueByPath("event.payload.spotter_phrase", spotterPhrase);
                }
                TMessage message(TMessage::FromClient, std::move(jsonVal));
                wsEvent.MutableContext()->SetSessionId("TODO:cuttlefish_client_session");
                wsEvent.MutableContext()->SetInitialMessageId("ffffffff-ffff-ffff-ffff-ffffffff0000");
                MessageToWsEvent(message, wsEvent);
                request->SendAudio(filenameSpotter, filename, mimeFormat, chunkSize, TDuration::MilliSeconds(chunkDuration), wsEvent);
            } else if (method == "asr.response") {
                //TODO: use audio here?
                NAlice::NAsr::NProtobuf::TResponse response;
                //response.MutableInitResponse()->SetIsOk(false);
                if (error) {
                    jsonVal.SetValueByPath("InitResponse.IsOk", false);
                    jsonVal.SetValueByPath("InitResponse.ErrMsg", error);
                }
                NProtobufJson::Json2Proto(jsonVal, response);
                {
                    NJson::TJsonValue j;
                    NProtobufJson::Proto2Json(response, j);
                    //Cout << WriteJson(j, false) << Endl;
                }
                request->SendAsrResponse(response);
            } else if (method == "tts.generate") {
                jsonVal.SetValueByPath("event.header.messageId", "ffffffff-ffff-ffff-ffff-ffffffff0000");
                TMessage message(TMessage::FromClient, std::move(jsonVal));
                MessageToWsEvent(message, wsEvent);
                request->SendEvent(wsEvent);
                //TMessage message(TMessage::FromClient, std::move(jsonVal));
                //MessageToWsEvent(message, wsEvent);
                //request->SendAudio(filenameSpotter, filename, mimeFormat, chunkSize, TDuration::MilliSeconds(chunkDuration), wsEvent);
/*            } else if (method == "audio") {
                NAlice::NAsr::NProtobuf::TResponse response;
                //response.MutableInitResponse()->SetIsOk(false);
                if (error) {
                    jsonVal.SetValueByPath("InitResponse.IsOk", false);
                    jsonVal.SetValueByPath("InitResponse.ErrMsg", error);
                }
                NProtobufJson::Json2Proto(jsonVal, response);
                {
                    NJson::TJsonValue j;
                    NProtobufJson::Proto2Json(response, j);
                    //Cout << WriteJson(j, false) << Endl;
                }
                request->SendAsrResponse(response);
*/
            }
        }
        eventFinish.WaitI();  // TODO: timeout?
        return 0;
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return 1;
    }
}
