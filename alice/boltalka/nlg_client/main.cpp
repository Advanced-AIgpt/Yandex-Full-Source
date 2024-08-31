#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/http/simple/http_client.h>

#include <util/charset/wide.h>
#include <util/generic/deque.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/string/vector.h>

void Communicate(const TSimpleHttpClient &httpClient,
                 const TCgiParameters &constParams,
                 ui64 maxContextLength,
                 bool quiet) {

    TDeque<TUtf16String> context;
    if (!quiet) {
        Cout << "> ";
    }
    for (TUtf16String line; Cin.ReadLine(line);) {
        context.push_back(line);
        while (context.size() > maxContextLength) {
            context.pop_front();
        }
        TCgiParameters params = constParams;
        params.InsertUnescaped("context", WideToUTF8(JoinStrings(context.begin(), context.end(), u"\n")));

        TString response;
        {
            TStringOutput output(response);
            httpClient.DoGet("/respond?" + params.Print(), &output);
        }
        TUtf16String reply = UTF8ToWide(response);
        Cout << response << Endl;

        reply = ToWtring(TWtringBuf(reply).Before('\n'));
        if (reply.EndsWith(u" _EOS_")) {
            reply = reply.substr(0, reply.size() - strlen(" _EOS_"));
        }
        context.push_back(reply);
        if (!quiet) {
            Cout << "> ";
        }
    }
}

int main(int argc, char** argv) {
    using namespace NLastGetopt;
    TOpts opts = NLastGetopt::TOpts::Default();
    opts
        .AddHelpOption();
    opts
        .AddLongOption('h', "host")
        .OptionalArgument()
        .DefaultValue("localhost")
        .Help("Host of NLG server\n\n\n");
    opts
        .AddLongOption('p', "port")
        .OptionalArgument()
        .DefaultValue("1612")
        .Help("Port of NLG server\n\n\n");
    opts
        .AddLongOption("model")
        .OptionalArgument()
        .DefaultValue("encoder_decoder_c123")
        .Help("Name of NLG model\n\n\n");
    opts
        .AddLongOption("max_context")
        .OptionalArgument()
        .DefaultValue("3")
        .Help("Maximal length of the context supported by model\n\n\n");
    opts
        .AddLongOption("temperature")
        .OptionalArgument()
        .DefaultValue("0.7")
        .Help("Temperature for sampling\n\n\n");
    opts
        .AddLongOption("max_len")
        .OptionalArgument()
        .DefaultValue("50")
        .Help("Maximal length of the generated reply\n\n\n");
    opts
        .AddLongOption("num_samples")
        .OptionalArgument()
        .DefaultValue("1")
        .Help("Number of samples in model response."
              " You are supposed to reply to the first one."
              " All others are for refernce only\n\n\n");
    opts
        .AddLongOption('q', "quiet")
        .NoArgument()
        .Help("Do not output \">\"\n\n\n");

    TOptsParseResult res(&opts, argc, argv);

    TSimpleHttpClient httpClient(res.Get("host"), res.Get<int>("port"), TDuration::Seconds(10));

    TCgiParameters constParams;
    for (const char *param : { "model", "temperature", "max_len", "num_samples" }) {
        constParams.InsertUnescaped(param, res.Get(param));
    }

    Communicate(httpClient, constParams, res.Get<ui64>("max_context"), res.Has("quiet"));

    return 0;
}
