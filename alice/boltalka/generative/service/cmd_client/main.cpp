#include <alice/boltalka/generative/service/proto/generative_request.pb.h>
#include <alice/boltalka/generative/service/proto/generative_response.pb.h>

#include <alice/boltalka/libs/text_utils/context_transform.h>

#include <alice/library/json/json.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/http/simple/http_client.h>
#include <library/cpp/langs/langs.h>

#include <util/string/split.h>
#include <util/system/hp_timer.h>

const TString PROTOBUF_CONTENT_TYPE = "application/protobuf";
const THashMap<TString, TString> HEADERS {
    {"Content-Type", PROTOBUF_CONTENT_TYPE},
    {"Accept",       PROTOBUF_CONTENT_TYPE}
};

NGenerativeBoltalka::Proto::TGenerativeResponse GetResponses(const TStringBuf uri, const TVector<TString>& contexts, size_t numHypos, const TMaybe<ui64> seed) {
    TStringBuf host;
    TStringBuf path;
    SplitUrlToHostAndPath(uri, host, path);
    TSimpleHttpClient client{TSimpleHttpClient::TOptions(host)};

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

    TStringStream res;
    client.DoPost(path, request.SerializeAsString(), &res, HEADERS);
    NGenerativeBoltalka::Proto::TGenerativeResponse response;
    if (!response.ParseFromString(res.Str())) {
        ythrow yexception() << "Could not parse response from server";
    }

    return response;
}

int main(int argc, const char *argv[]) {
    TString uri;
    size_t numHypos;
    TMaybe<ui64> seed = Nothing();

    NLastGetopt::TOpts opts;
    opts.AddLongOption("uri")
            .Help("Uri to send requests to")
            .Required()
            .StoreResult(&uri);
    opts.AddLongOption("num-hypos")
            .Help("Number of hypothesis to ask from the server per request")
            .Optional()
            .StoreResult(&numHypos)
            .DefaultValue(1);
    opts.AddLongOption("seed")
            .Help("Seed to pass with requests (if not specified, then no seed) (ui64 type)")
            .Handler1T<ui64>([&] (ui64 value) {
                seed = value;
            });
    opts.SetFreeArgsNum(0, 0);

    NLastGetopt::TOptsParseResult optsParseResult(&opts, argc, argv);
    auto preprocessTransform = NNlgTextUtils::TNlgSearchContextTransform(LanguageByName("ru"));
    Cerr << "Context is tab separated: context_0\\tcontext_1\\tcontext_2" << Endl;

    while (TString line = Cin.ReadLine()) {
        if (line.empty()) {
            continue;
        }

        TVector<TString> context;
        StringSplitter(line).Split('\t').Collect(&context);
        TVector<TString> preprocessedContext = preprocessTransform.Transform(context);
        THPTimer watch;
        const auto& result = GetResponses(uri, context, numHypos, seed);
        Cout << NAlice::JsonFromProto(result) << "\t" << watch.Passed() << Endl;
    }

    return 0;
}
