#include <alice/boltalka/generative/service/proto/generative_request.pb.h>
#include <alice/boltalka/generative/service/proto/generative_response.pb.h>

#include <alice/boltalka/generative/inference/core/data.h>
#include <alice/boltalka/libs/text_utils/context_transform.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/http/simple/http_client.h>
#include <library/cpp/langs/langs.h>
#include <library/cpp/threading/future/async.h>

#include <util/system/hp_timer.h>

using namespace NGenerativeBoltalka;

const TString PROTOBUF_CONTENT_TYPE = "application/protobuf";

std::pair<TVector<TGenerativeResponse>, float> GetResponsesWithTiming(
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
    client.DoPost(handlerPath, request.SerializeAsString(), &stream, {
        {"Content-Type", PROTOBUF_CONTENT_TYPE},
        {"Accept",       PROTOBUF_CONTENT_TYPE}
    });
    NGenerativeBoltalka::Proto::TGenerativeResponse response;
    if (!response.ParseFromString(stream.Str())) {
        ythrow yexception() << "Could not parse response from server";
    }

    TVector<TGenerativeResponse> responses;
    for (const auto& r: response.GetResponses()) {
        responses.push_back(TGenerativeResponse(r.GetResponse(), r.GetScore(), r.GetNumTokens()));
    }

    float timing = watch.Passed();
    return std::make_pair(responses, timing);
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
    size_t numRequestsPerThread;
    opts.AddLongOption("num-requests-per-thread")
            .Help("Number of requests to make")
            .Required()
            .StoreResult(&numRequestsPerThread);
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
    NLastGetopt::TOptsParseResult optsParseResult(&opts, argc, argv);

    auto preprocessTransform = NNlgTextUtils::TNlgSearchContextTransform(LanguageByName("ru"));

    TVector<TString> context = {"поговори со мной мне грустно", "Да да? я вас слушаю", "алиска"};
    TVector<TString> preprocessedContext = preprocessTransform.Transform(context);

    auto pool = CreateThreadPool(numLoadThreads);

    THPTimer watch;
    TVector<NThreading::TFuture<TVector<std::pair<TVector<TGenerativeResponse>, float>>>> resultFutures;
    for (size_t i = 0; i < numLoadThreads; i++) {
        auto future = NThreading::Async(
                [host, port, handlerPath, numHypos, preprocessedContext, seed, numRequestsPerThread]() {
                    TSimpleHttpClient client(host, port);

                    TVector<std::pair<TVector<TGenerativeResponse>, float>> results;
                    for (size_t i = 0; i < numRequestsPerThread; i++) {
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

    for (const auto& f : resultFutures) {
        for (const auto& pair : f.GetValueSync()) {
            auto[responses, time] = pair;

            sumRequestResponsesTime += time;
            totalRequestResponses++;

            if (responses.size() == 0) {
                totalEmptyRequestResponses++;
            }

            for (auto& response : responses) {
                TVector<TString> responseWords;
                Split(response.Response, " ", responseWords);
                totalResponsesLen += responseWords.size();
                totalResponses++;
            }
        }
    }
    float totalTime = watch.Passed();

    Cerr << "Total responses: " << totalRequestResponses << Endl;
    Cerr << "Approximate RPS: " << float(totalRequestResponses) / totalTime << Endl;
    Cerr << "Mean time for request: " << float(sumRequestResponsesTime) / totalRequestResponses << Endl;
    Cerr << "Empty responses ratio: " << float(totalEmptyRequestResponses) / totalRequestResponses << Endl;
    Cerr << "Mean responses per request: : " << float(totalResponses) / totalRequestResponses << Endl;
    Cerr << "Mean words per response: : " << float(totalResponsesLen) / totalResponses << Endl;

    return 0;
}
