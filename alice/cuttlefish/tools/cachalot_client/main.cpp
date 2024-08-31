#include <alice/cuttlefish/library/api/handlers.h>
#include <alice/cuttlefish/library/cachalot_client/cachalot_client.h>
#include <alice/cuttlefish/library/logging/dlog.h>
#include <voicetech/library/proto_api/yabio.pb.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/protobuf/json/json2proto.h>

#include <util/system/event.h>
#include <util/system/hostname.h>

namespace NDebug {
    static bool Enable = false;
}

namespace {
    void DebugOut(const TString& s) {
        if (NDebug::Enable) {
            Cerr << TInstant::Now() << " " << s << Endl;
        }
    }
}

using namespace NAlice;

class TCachalotRequest: public TCachalotClient::TRequest {
public:
    TCachalotRequest(TCachalotClient& cachalotClient, NAsio::TIOService& ioService, TManualEvent& event, bool appHostGraph)
        : TCachalotClient::TRequest(cachalotClient, ioService)
        , Event_(event)
        , AppHostGraph_(appHostGraph)
    {
    }
    ~TCachalotRequest() override {
        DebugOut("~TCachalotRequest");
        Event_.Signal();
    }

    void SendYabioContextRequest2(const TString& method, const TString& groupId, const TString& devModel, const TString& devManuf, const YabioProtobuf::YabioContext* context) {
        NCachalotProtocol::TYabioContextRequest request;
        NCachalotProtocol::TYabioContextKey key;
        key.SetGroupId(groupId);
        key.SetDevModel(devModel);
        key.SetDevManuf(devManuf);
        if (method == "put" || method == "save") {
            if (!context) {
                ythrow yexception() << "for saving yabio_context required context data";
            }

            TString contextStr;
            Y_PROTOBUF_SUPPRESS_NODISCARD context->SerializeToString(&contextStr);
            auto& save = *request.MutableSave();
            *save.MutableKey() = key;
            DebugOut(TStringBuilder() << "Send YabioContextRequest (-log context): " << request.ShortUtf8DebugString());
            save.SetContext(contextStr);
        } else if (method == "get" || method == "load") {
            *request.MutableLoad()->MutableKey() = key;
            DebugOut(TStringBuilder() << "Send YabioContextRequest: " << request.ShortUtf8DebugString());
        } else if (method == "del" || method == "delete") {
            *request.MutableDelete()->MutableKey() = key;
            DebugOut(TStringBuilder() << "Send YabioContextRequest: " << request.ShortUtf8DebugString());
        } else {
            throw yexception() << "unknown yabio_context method: " << method;
        }
        SendYabioContextRequest(std::move(request), AppHostGraph_);
    }

private:
    void OnYabioContextResponse(NCachalotProtocol::TYabioContextResponse& response) override {
        if (response.HasSuccess()) {
            if (response.GetSuccess().HasContext()) {
                YabioProtobuf::YabioContext context;
                TString s = response.GetSuccess().GetContext();
                response.MutableSuccess()->ClearContext();
                DebugOut(TStringBuilder() << "got YabioContextResponse: " << response.ShortUtf8DebugString());
                try {
                    // parse response.GetSuccess().GetContext(), print HR version
                    Y_PROTOBUF_SUPPRESS_NODISCARD context.ParseFromString(s);
                    DebugOut(TStringBuilder() << "YabioContext: " << context.ShortUtf8DebugString());
                    NJson::TJsonValue json;
                    {
                        using namespace NProtobufJson;
                        TProto2JsonConfig cfg;
                        cfg.SetMissingSingleKeyMode(TProto2JsonConfig::MissingKeyDefault);
                        Proto2Json(context, json, cfg);
                    }
                    Cout << WriteJson(json, false) << Endl;
                } catch (...) {
                    OnError(CurrentExceptionMessage());
                }
            } else {
                DebugOut(TStringBuilder() << "got YabioContextResponse: " << response.ShortUtf8DebugString());
            }
        } else if (response.HasError()) {
            DebugOut(TStringBuilder() << "got YabioContextResponse: " << response.ShortUtf8DebugString());
            OnError(TStringBuilder() << "got error response: " << response.ShortUtf8DebugString());
        } else {
            DebugOut(TStringBuilder() << "got YabioContextResponse: " << response.ShortUtf8DebugString());
            OnError("got YabioContextResponse with undefined payload (!Error & !Success)");
        }
    }
    bool NextAsyncRead() override {
        if (TCachalotClient::TRequest::NextAsyncRead()) {
            DebugOut("NextAsyncRead");
            return true;
        }
        return false;
    }
    void OnEndNextResponseProcessing() override {
        TCachalotClient::TRequest::OnEndNextResponseProcessing();
    }
    void OnEndOfResponsesStream() override {
        DebugOut("EndOfResponsesStream");
    }
    void OnTimeout() override {
        TCachalotClient::TRequest::OnTimeout();
    }
    void OnError(const TString& s) override {
        DebugOut(TStringBuilder() << "OnError: " << s);
        TCachalotClient::TRequest::OnError(s);
    }

private:
    TManualEvent& Event_;
    bool AppHostGraph_;
};


int main(int argc, char *argv[]) {
    using namespace NLastGetopt;
    using namespace NAlice::NCuttlefish;
    TOpts opts = NLastGetopt::TOpts::Default();
    opts.AddLongOption("debug").StoreResult(&NDebug::Enable);
    TCachalotClient::TConfig config;
    config.Host = "localhost";
    config.Port = 8081;
    config.Path = "-";
    config.Timeout = TDuration::Seconds(100);
    opts.AddLongOption("host").StoreResult(&config.Host);
    opts.AddLongOption("path").StoreResult(&config.Path);
    opts.AddLongOption("port").StoreResult(&config.Port);
    bool appHostGraph = false;  // use direct request to backend instead apphost graph
    opts.AddLongOption("apphost-graph").StoreResult(&appHostGraph);
    TString service;  // yabio_context|...
    opts.AddLongOption("service").StoreResult(&service);
    TString method;  // save|load|delete
    opts.AddLongOption("method").StoreResult(&method);
    TString file;  // common file
    opts.AddLongOption("file").StoreResult(&file);
    TString jsonFile;  // common file
    opts.AddLongOption("json-file").StoreResult(&jsonFile);
    // yabio_context params
    TString groupId;
    opts.AddLongOption("group-id").StoreResult(&groupId);
    TString devModel;
    opts.AddLongOption("dev-model").StoreResult(&devModel);
    TString devManuf;
    opts.AddLongOption("dev-manuf").StoreResult(&devManuf);

    TOptsParseResult res(&opts, argc, argv);
    // flexible defaults
    if (service == "activation") {
        if (config.Path == "-") {
            config.Path = SERVICE_HANDLE_ACTIVATION;
        }
    } else if (service == "cache") {
        if (config.Path == "-") {
            config.Path = SERVICE_HANDLE_CACHE;
        }
    } else if (service == "location") {
        if (config.Path == "-") {
            config.Path = SERVICE_HANDLE_LOCATION;
        }
    } else if (service == "mm_session") {
        if (config.Path == "-") {
            config.Path = SERVICE_HANDLE_MM_SESSION;
        }
    } else if (service == "yabio_context") {
        if (config.Path == "-") {
            config.Path = SERVICE_HANDLE_YABIO_CONTEXT;
        }
    } // TODO: more services
    NAsio::TExecutorsPool pool(1);

    try {
        TManualEvent eventFinish;
        TCachalotClient cachalotClient(config);
        {
            TIntrusivePtr<TCachalotRequest> cachalotRequest(new TCachalotRequest(cachalotClient, pool.GetExecutor().GetIOService(), eventFinish, appHostGraph));
            if (service == "activation") {
                Cout << "TODO" << Endl;
            } else if (service == "cache") {
                Cout << "TODO" << Endl;
            } else if (service == "location") {
                Cout << "TODO" << Endl;
            } else if (service == "mm_session") {
                Cout << "TODO" << Endl;
            } else if (service == "yabio_context") {
                YabioProtobuf::YabioContext context;
                const YabioProtobuf::YabioContext* contextPtr = nullptr;
                if (file) {
                    TFileInput f(file);
                    Y_PROTOBUF_SUPPRESS_NODISCARD context.ParseFromString(f.ReadAll());
                    contextPtr = &context;
                } else if (jsonFile) {
                    TFileInput f(jsonFile);
                    TString content = f.ReadAll();
                    NJson::TJsonValue json = NJson::ReadJsonFastTree(content);
                    {
                        using namespace NProtobufJson;
                        Json2Proto(json, context);
                    }
                    contextPtr = &context;
                }
                cachalotRequest->SendYabioContextRequest2(method, groupId, devModel, devManuf, contextPtr);
            } // TODO: more services
        }
        eventFinish.WaitI();  // TODO: timeout?
        return 0;
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return 1;
    }
}
