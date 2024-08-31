#include <alice/boltalka/generative/service/proto/generative_request.pb.h>
#include <alice/boltalka/generative/service/proto/generative_response.pb.h>
#include <alice/boltalka/generative/service/proto/bert_request.pb.h>
#include <alice/boltalka/generative/service/proto/bert_response.pb.h>

#include <alice/boltalka/libs/text_utils/context_transform.h>

#include <alice/library/proto/proto.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/langs/langs.h>
#include <library/cpp/string_utils/url/url.h>
#include <library/cpp/getopt/modchooser.h>

#include <util/string/builder.h>
#include <util/string/split.h>
#include <util/string/join.h>
#include <util/digest/city.h>

const TString PROTOBUF_CONTENT_TYPE = "application/protobuf";

TString BuildRequest(const TStringBuf uri, const TStringBuf requestBody) {
    TStringBuf host;
    TStringBuf path;
    SplitUrlToHostAndPath(uri, host, path);
    TStringBuilder builder;
    builder << "POST " << path << " HTTP/1.1" << Endl;
    builder << "Content-Length: " << requestBody.size() << Endl;
    builder << "Content-Type: " << PROTOBUF_CONTENT_TYPE << Endl;
    builder << "Host: " << host << Endl;;
    builder << Endl;
    builder << requestBody << Endl;
    return builder;
}

void PrintAmmoBert(const TStringBuf uri, const TVector<TString>& contexts, const TVector<TString>& candidates) {
    NGenerativeBoltalka::Proto::TBertFactorRequest request;
    auto* requestContext = request.MutableContext();
    for (auto& context : contexts) {
        requestContext->Add(TString(context));
    }

    auto* requestCandidates = request.MutableCandidates();
    for (auto& candidate : candidates) {
        requestCandidates->Add(TString(candidate));
    }

    const auto requestBody = request.SerializeAsString();
    const auto requestString = BuildRequest(uri, requestBody);

    Cout << requestString.size() << Endl;
    Cout << requestString << Endl;
    Cout << Endl;
}

void PrintAmmoGenerative(const TStringBuf uri, const TVector<TString>& contexts, size_t numHypos, const TMaybe<ui64> seed) {
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

    const auto requestBody = request.SerializeAsString();
    const auto requestString = BuildRequest(uri, requestBody);

    Cout << requestString.size() << Endl;
    Cout << requestString << Endl;
    Cout << Endl;
}

int main_bert(int argc, const char *argv[]) {
    TString uri;
    TString lang;

    NLastGetopt::TOpts opts;
    opts.AddLongOption("uri")
            .Help("Uri to send requests to")
            .Required()
            .StoreResult(&uri);
    opts.AddLongOption("lang")
            .Help("Language for preprocessing")
            .Optional()
            .DefaultValue("ru")
            .StoreResult(&lang);
    opts.SetFreeArgsNum(0, 0);

    NLastGetopt::TOptsParseResult optsParseResult(&opts, argc, argv);
    auto preprocessTransform = NNlgTextUtils::TNlgSearchContextTransform(LanguageByName(lang));

    TString line;
    TVector<TString> context;
    TVector<TString> candidates;
    ui64 prevContextHash = 0;
    size_t contextIdx = 0;
    while (Cin.ReadLine(line)) {
        if (line.empty()) {
            continue;
        }
        TVector<TString> turns;
        StringSplitter(line).Split('\t').Collect(&turns);
        auto preprocessedTurns = preprocessTransform.Transform(turns);
        Y_ASSERT(preprocessedTurns.size() > 1);
        context.clear();
        std::move(preprocessedTurns.begin(), preprocessedTurns.end() - 1, std::back_inserter(context));
        ui64 contextHash = CityHash64(JoinSeq("\t", context));
        if (contextIdx == 0 && candidates.size() == 0) {
            prevContextHash = contextHash;
        }
        if (contextHash != prevContextHash) {
            prevContextHash = contextHash;
            PrintAmmoBert(uri, context, candidates);
            Cerr << contextIdx++ << " " << candidates.size() << "\n";
            candidates.clear();
        }
        candidates.push_back(preprocessedTurns.back());
    }
    PrintAmmoBert(uri, context, candidates);
    Cerr << contextIdx++ << " " << candidates.size() << "\n";

    return 0;
}

int main_generative(int argc, const char *argv[]) {
    TString uri;
    TString lang;
    size_t numHypos;
    TMaybe<ui64> seed = Nothing();
    TMaybe<size_t> contextLength = Nothing();

    NLastGetopt::TOpts opts;
    opts.AddLongOption("uri")
            .Help("Uri to send requests to")
            .Required()
            .StoreResult(&uri);
    opts.AddLongOption("lang")
            .Help("Language for preprocessing")
            .Optional()
            .DefaultValue("ru")
            .StoreResult(&lang);
    opts.AddLongOption("num-hypos")
            .Help("Number of hypothesis to ask from the server per request")
            .Optional()
            .DefaultValue(1)
            .StoreResult(&numHypos);
    opts.AddLongOption("seed")
            .Help("Seed to pass with requests (if not specified, then no seed) (ui64 type)")
            .Handler1T<ui64>([&] (ui64 value) {
                seed = value;
            });
    opts.AddLongOption("context-length")
            .Help("If specified, contexts will be truncated to specified size")
            .Handler1T<size_t>([&] (size_t value) {
                contextLength = value;
            });
    opts.SetFreeArgsNum(0, 0);

    NLastGetopt::TOptsParseResult optsParseResult(&opts, argc, argv);
    auto preprocessTransform = NNlgTextUtils::TNlgSearchContextTransform(LanguageByName(lang));
    Cerr << "Context is tab separated (example: context_0\\tcontext_1\\tcontext_2)" << Endl;

    TString line;
    while (Cin.ReadLine(line)) {
        if (line.empty()) {
            continue;
        }
        TVector<TString> context;
        StringSplitter(line).Split('\t').Collect(&context);

        if (contextLength.Defined() && context.size() > contextLength.GetRef()) {
            context.erase(context.begin() + contextLength.GetRef(), context.begin() + context.size());
        }

        TVector<TString> preprocessedContext = preprocessTransform.Transform(context);
        PrintAmmoGenerative(uri, preprocessedContext, numHypos, seed);
    }

    return 0;
}

int main(int argc, const char *argv[]) {
    TModChooser modChooser;

    modChooser.AddMode(
        "generative",
        main_generative,
        "build ammo for generative server");

    modChooser.AddMode(
        "bert",
        main_bert,
        "build ammo for bert server");

    return modChooser.Run(argc, argv);
}
