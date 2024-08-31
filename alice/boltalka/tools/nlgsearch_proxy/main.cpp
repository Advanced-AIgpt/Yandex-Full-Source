#include <kernel/server/protos/serverconf.pb.h>
#include <kernel/server/server.h>

#include <library/cpp/getopt/small/last_getopt.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/neh/neh.h>

#include <util/string/cast.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/stream/str.h>

class TNlgSearchRequest : public NServer::TRequest {
public:
    struct TOptions {
        ui32 NumRetries;
        TDuration Timeout;
    };

    TNlgSearchRequest(NServer::TServer& server,
                      const TString& nlgSearchHost,
                      ui32 nlgSearchPort,
                      const TOptions& opts)
        : NServer::TRequest{server}
        , NlgSearchHost(nlgSearchHost)
        , NlgSearchPort(nlgSearchPort)
        , Opts(opts)
    {
    }

    THttpResponse ProcessRequest() {
        const auto& cgi = RequestData().CgiParam;
        TString query;
        if (!cgi.Has("text")) {
            return FAIL;
        }
        query = cgi.Get("text");

        size_t maxResults = 1;
        if (cgi.Has("max-results")) {
            TryFromString<size_t>(cgi.Get("max-results"), maxResults);
        }
        const size_t maxMaxResults = 1000;
        maxResults = Min(maxResults, maxMaxResults);

        TString url = BuildNlgSearchUrl(query, maxResults);
        TDuration totalDuration(TDuration::MilliSeconds(0));
        const TDuration minDuration = TDuration::MilliSeconds(250);
        for (size_t attempt = 0; attempt < Opts.NumRetries; ++attempt) {
            NNeh::TResponseRef response;
            try {
                response = NNeh::Request(url)->Wait(Opts.Timeout - totalDuration);
            } catch (...) {
            }
            if (!response) {
                continue;
            }
            if (response->IsError()) {
                totalDuration += response->Duration;
                if (totalDuration + minDuration > Opts.Timeout) {
                    break;
                }
            }
            TString textResponse;
            if (!ParseResponse(response->Data, &textResponse)) {
                return FAIL;
            }
            return TextResponse(textResponse);
        }
        return FAIL;
    }

    bool DoReply(const TString& path, THttpResponse& result) override {
        if (path == "/ping") {
            result = THttpResponse{HTTP_OK};
            return true;
        }
        if (path != "/nlgsearch") {
            return false;
        }
        result = ProcessRequest();
        return true;
    }

private:
    static bool ParseResponse(TStringBuf data, TString* result) {
        try {
            NJson::TJsonValue responseJson;
            if (!NJson::ReadJsonTree(data, &responseJson)) {
                return false;
            }
            if (!responseJson.Has("Grouping")) {
                return false;
            }
            if (responseJson["Grouping"].GetArraySafe().empty()) {
                return false;
            }
            if (!responseJson["Grouping"].GetArraySafe().front().Has("Group")) {
                return false;
            }
            NJson::TJsonValue jsonResult{NJson::JSON_ARRAY};
            const auto& groups = responseJson["Grouping"].GetArraySafe().front()["Group"].GetArraySafe();
            for (const auto& group : groups) {
                const auto& doc = group["Document"][0];
                NJson::TJsonValue outDoc;
                outDoc["score"] = doc["Relevance"].GetUIntegerSafe();
                const auto& attrs = doc["ArchiveInfo"]["GtaRelatedAttribute"].GetArraySafe();
                for (const auto& attr : attrs) {
                    if (attr["Key"].GetStringSafe() == "reply") {
                        outDoc["text"] = attr["Value"].GetStringSafe();
                        break;
                    }
                }
                jsonResult.AppendValue(outDoc);
            }
            *result = WriteJson(jsonResult);
        } catch (...) {
            result->clear();
            return false;
        }
        return true;
    }

    TString BuildNlgSearchUrl(const TString& query, size_t maxResults) const {
        TStringStream url;
        url << "http://" << NlgSearchHost << ":" << NlgSearchPort << "/yandsearch?g=0..100&ms=proto&hr=json&";
        TCgiParameters cgi;
        cgi.InsertUnescaped("text", query);
        cgi.InsertUnescaped("relev", GetRelevParams(maxResults));
        url << cgi.Print();
        return url.Str();
    }

    static TString GetRelevParams(size_t maxResults) {
        TStringStream relev;
        relev << "MaxResults=" << maxResults
            << ";MinRatioWithBestResponse=" << 1
            << ";SearchFor=" << "context_and_reply"
            << ";SearchBy=" << "context"
            << ";DssmModelName=" << "insight_c3_rus_lister"
            << ";ContextWeight=" << 1
            << ";RankerModelName=" << "catboost";
        return relev.Str();
    }

private:
    const TString NlgSearchHost;
    const ui32 NlgSearchPort;
    const TOptions Opts;
    static const THttpResponse FAIL;
};

const THttpResponse TNlgSearchRequest::FAIL{HTTP_INTERNAL_SERVER_ERROR};

class TNlgSearchProxyServer : public NServer::TServer {
public:
    TNlgSearchProxyServer(const NServer::THttpServerConfig& config,
                          const TString& nlgSearchHost,
                          ui32 nlgSearchPort,
                          const TNlgSearchRequest::TOptions& opts)
        : NServer::TServer{config}
        , NlgSearchHost(nlgSearchHost)
        , NlgSearchPort(nlgSearchPort)
        , Opts(opts)
    {
    }

    TClientRequest* CreateClient() override {
        return new TNlgSearchRequest(*this, NlgSearchHost, NlgSearchPort, Opts);
    }

private:
    const TString NlgSearchHost;
    const ui32 NlgSearchPort;
    const TNlgSearchRequest::TOptions Opts;
};

int main(int argc, const char* argv[]) {
    NServer::THttpServerConfig config;
    config.SetConfigCheckDelay(365 * 24 * 60);

    TString nlgSearchHost;
    ui32 nlgSearchPort;
    TNlgSearchRequest::TOptions requestOpts;

    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
    opts
        .AddHelpOption();
    opts
        .AddLongOption('H', "nlgsearch-host")
        .RequiredArgument("HOST")
        .DefaultValue("general-conversation.yandex.net")
        .StoreResult(&nlgSearchHost)
        .Help("nlgsearch host.");
    opts
        .AddLongOption('P', "nlgsearch-port")
        .RequiredArgument("INT")
        .DefaultValue("80")
        .StoreResult(&nlgSearchPort)
        .Help("nlgsearch port.");
    opts
        .AddLongOption('r', "retries")
        .RequiredArgument("INT")
        .DefaultValue("3")
        .StoreResult(&requestOpts.NumRetries)
        .Help("Num retries");
    opts
        .AddLongOption('t', "timeout")
        .RequiredArgument("INT")
        .DefaultValue("3000")
        .Handler1T<ui32>([&requestOpts](ui32 timeout) {
            requestOpts.Timeout = TDuration::MilliSeconds(timeout);
        })
        .Help("Request timeout in ms");
    opts
        .AddLongOption('p', "port")
        .RequiredArgument("INT")
        .DefaultValue("80")
        .Handler1T<ui32>([&config](ui32 port) {
            config.SetPort(port);
        })
        .Help("HTTP-server port.");
    opts
        .AddLongOption('T', "num-threads")
        .RequiredArgument("INT")
        .DefaultValue("4")
        .Handler1T<ui32>([&config](ui32 numThreads) {
            config.SetThreads(numThreads);
        })
        .Help("Num serving threads.");
    opts
        .AddLongOption('q', "queue-size")
        .RequiredArgument("INT")
        .DefaultValue("16")
        .Handler1T<ui32>([&config](ui32 queueSize) {
            config.SetMaxQueueSize(queueSize);
        })
        .Help("Queue size.");

    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');

    NLastGetopt::TOptsParseResult parsedOpts(&opts, argc, argv);

    TNlgSearchProxyServer server(config, nlgSearchHost, nlgSearchPort, requestOpts);
    server.Start();
    server.Wait();

    return 0;
}
