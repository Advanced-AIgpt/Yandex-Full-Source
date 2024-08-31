#include <alice/boltalka/generative/service/proto/generative_request.pb.h>
#include <alice/boltalka/generative/service/proto/generative_response.pb.h>

#include <alice/boltalka/generative/inference/core/data.h>
#include <alice/boltalka/libs/text_utils/context_transform.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/http/simple/http_client.h>
#include <library/cpp/langs/langs.h>
#include <library/cpp/threading/future/async.h>

#include <util/stream/file.h>
#include <util/system/hp_timer.h>

using namespace NGenerativeBoltalka;

const TString PROTOBUF_CONTENT_TYPE = "application/protobuf";

std::tuple<TVector<TGenerativeResponse>, float, bool> GetResponsesWithTiming(
        const TVector<TString>& contexts, size_t numHypos, TSimpleHttpClient& client,
        const TString& handlerPath, TMaybe<ui64> seed = Nothing()) {
    THPTimer watch;

    NGenerativeBoltalka::Proto::TGenerativeRequest request;
    auto* requestContext = request.MutableContext();
    for (auto& context : contexts) {
        requestContext->Add(TString(context));
    }
    request.SetNumHypos(numHypos);

    if (seed.Defined()) {
        auto* seedMsg = request.MutableSeed();
        seedMsg->SetValue(seed.GetRef());
    }
    TStringStream stream;
    try {
        client.DoPost(handlerPath, request.SerializeAsString(), &stream, {
                {"Content-Type", PROTOBUF_CONTENT_TYPE},
                {"Accept",       PROTOBUF_CONTENT_TYPE}
        });
        NGenerativeBoltalka::Proto::TGenerativeResponse response;
        if (!response.ParseFromString(stream.Str())) {
            ythrow
            yexception() << "Could not parse response from server";
        }

        TVector <TGenerativeResponse> responses;
        for (const auto &r: response.GetResponses()) {
            responses.push_back(TGenerativeResponse(r.GetResponse(), r.GetScore(), r.GetNumTokens()));
        }

        float timing = watch.Passed();
        return std::make_tuple(responses, timing, false);
    } catch (...) {
        Cerr << "WARN: Got 500" << Endl;
        TVector <TGenerativeResponse> responses;
        float timing = watch.Passed();
        return std::make_tuple(responses, timing, true);
    }
}

float CalculateQuantile(TVector<float>& values, float quantile) {
    int valueId = int(float(values.size()) * quantile);
    std::nth_element(values.begin(), values.begin() + valueId, values.end());
    return values[valueId];
}

int main(int argc, const char *argv[]) {
    NLastGetopt::TOpts opts;

    TString host;
    opts.AddLongOption("server-host")
            .Help("Host to send requests to")
            .Required()
            .StoreResult(&host);
    int port;
    opts.AddLongOption("server-port")
            .Help("Port number for the server")
            .Required()
            .StoreResult(&port);
    TString handlerPath;
    opts.AddLongOption("server-handler-path")
            .Help("Handler path for requests on the server")
            .Optional()
            .StoreResult(&handlerPath)
            .DefaultValue("/generative");
    size_t numHypos;
    opts.AddLongOption("num-hypos")
            .Help("Number of hypothesis to ask from the server per request")
            .Optional()
            .StoreResult(&numHypos)
            .DefaultValue(1);
    size_t numLoadThreads;
    opts.AddLongOption("num-load-threads")
            .Help("Number of threads to use to send requests (each thread will use its own client)")
            .Required()
            .StoreResult(&numLoadThreads);
    TMaybe<ui64> seed = Nothing();
    opts.AddLongOption("seed")
            .Help("Seed to pass with requests (if not specified, then no seed) (ui64 type)")
            .Handler1T<ui64>([&] (ui64 value) {
                seed = value;
            });
    TString inputFile;
    opts.AddLongOption("input-path")
            .Help("Input tsv path of contexts")
            .Required()
            .StoreResult(&inputFile);
    size_t maxRequests;
    opts.AddLongOption("max-requests")
            .Help("Max total requests to send (0 means all requests from the input file)")
            .Optional()
            .StoreResult(&maxRequests)
            .DefaultValue(0);
    NLastGetopt::TOptsParseResult optsParseResult(&opts, argc, argv);

    auto preprocessTransform = NNlgTextUtils::TNlgSearchContextTransform(LanguageByName("ru"));
    TVector<TVector<TVector<TString>>> preprocessedContextsPerThread(numLoadThreads);

    TFileInput f(inputFile);
    TString line;

    size_t id = 0;
    while (f.ReadLine(line)) {
        TVector<TString> context;
        StringSplitter(line).Split('\t').Collect(&context);
        TVector<TString> preprocessedContext = preprocessTransform.Transform(context);
        TVector<TString> reversedContext(preprocessedContext);
        std::reverse(reversedContext.begin(), reversedContext.end());
        preprocessedContextsPerThread[id % numLoadThreads].push_back(reversedContext);
        id++;

        if (maxRequests != 0 and id >= maxRequests) {
            break;
        }
    }

    Cerr << "Making 10 requests to warmup the server." << Endl;
    for (size_t i = 0; i < 10; i++) {
        TSimpleHttpClient client{TSimpleHttpClient::TOptions(host).Port(port).SocketTimeout(TDuration::Seconds(3600))};
        GetResponsesWithTiming(preprocessedContextsPerThread[0][0], numHypos, client, handlerPath, seed);
    }

    Cerr << "Prepared total requests: " << id << Endl;
    Cerr << "Distribution of contexts between workers: " << Endl;
    for (auto& contexts : preprocessedContextsPerThread) {
        Cerr << "\tNum requests: " << contexts.size() << Endl;
    }

    auto pool = CreateThreadPool(numLoadThreads);

    THPTimer watch;
    TVector<NThreading::TFuture<TVector<std::tuple<TVector<TGenerativeResponse>, float, bool>>>> resultFutures;
    for (size_t curThread = 0; curThread < numLoadThreads; curThread++) {
        auto future = NThreading::Async(
                [host, port, handlerPath, numHypos, preprocessedContextsPerThread, seed, curThread]() {
                    TSimpleHttpClient client{TSimpleHttpClient::TOptions(host).Port(port).SocketTimeout(TDuration::Seconds(3600))};

                    TVector<std::tuple<TVector<TGenerativeResponse>, float, bool>> results;

                    for (auto& preprocessedContext : preprocessedContextsPerThread[curThread]) {
                        results.push_back(GetResponsesWithTiming(preprocessedContext, numHypos, client, handlerPath, seed));
                    }
                    return results;
                }, *pool);
        resultFutures.push_back(future);
    }

    float sumRequestResponsesTime = 0.0;
    size_t totalRequestResponses = 0;
    size_t totalResponses = 0;
    size_t totalResponsesLen = 0;
    size_t totalEmptyRequestResponses = 0;
    size_t total500 = 0;
    TVector<float> times;

    for (const auto& f : resultFutures) {
        for (const auto& pair : f.GetValueSync()) {
            auto[responses, time, is_500] = pair;

            if (!is_500) {
                sumRequestResponsesTime += time;
                times.push_back(time);
                totalRequestResponses++;

                if (responses.size() == 0) {
                    totalEmptyRequestResponses++;
                }

                for (auto &response : responses) {
                    TVector <TString> responseWords;
                    Split(response.Response, " ", responseWords);
                    totalResponsesLen += responseWords.size();
                    totalResponses++;
                }
            } else {
                total500++;
            }
        }
    }
    float totalTime = watch.Passed();

    Cerr << "Total 200 responses: " << totalRequestResponses << Endl;
    Cerr << "Total 5xx responses: " << total500 << Endl;
    Cerr << "Approximate RPS: " << float(totalRequestResponses) / totalTime << Endl;
    Cerr << "Mean time for request: " << float(sumRequestResponsesTime) / totalRequestResponses << Endl;
    Cerr << "Empty responses ratio: " << float(totalEmptyRequestResponses) / totalRequestResponses << Endl;
    Cerr << "Mean responses per request: : " << float(totalResponses) / totalRequestResponses << Endl;
    Cerr << "Mean words per response: : " << float(totalResponsesLen) / totalResponses << Endl;

    for (const auto& quantile : {0.5, 0.90, 0.99}) {
        Cerr << "Quantile " << quantile << ": " << float(CalculateQuantile(times, quantile)) << Endl;
    }

    return 0;
}
