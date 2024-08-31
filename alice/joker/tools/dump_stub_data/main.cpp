#include <alice/joker/library/stub/stub.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/http/misc/parsed_request.h>

#include <util/datetime/base.h>
#include <util/stream/file.h>
#include <library/cpp/cgiparam/cgiparam.h>

int main(int argc, const char** argv) {
    using namespace NAlice::NJoker;

    NLastGetopt::TOpts opts;
    opts.AddLongOption("request").Help("Dump request").Optional().HasArg(NLastGetopt::EHasArg::OPTIONAL_ARGUMENT);
    opts.SetFreeArgsNum(1);
    opts.SetFreeArgTitle(0, "<stub_file>", "stub proto file");
    NLastGetopt::TOptsParseResult res{&opts, argc, argv};

    const TString filename = res.GetFreeArgs()[0];
    TStubItemPtr stub;
    TFileInput file{filename};
    TStubId stubId{"", "", ""};
    if (const auto error = TStubItem::Load(stubId, file, stub)) {
        Cerr << "Unable to load stub from file: '" << filename << "': " << *error << Endl;
        return EXIT_FAILURE;
    }

    TStringBuf requestData = stub->Get().GetRequest().GetData();
    TParsedHttpFull req{requestData.NextTok('\n')};
    TCgiParameters cgi{req.Cgi};

    Cout << "Url: " << stub->Get().GetRequest().GetUrl() << Endl
         << "Path: " << req.Path << Endl
         << "Cgi: " << req.Cgi << Endl;
    TVector<TString> cgis(Reserve(cgi.size()));
    for (const auto& param : cgi) {
        cgis.emplace_back(TStringBuilder{} << "'" << param.first << "' = '" << param.second << "'");
    }
    Sort(cgis.begin(), cgis.end());
    for (const auto& s : cgis) {
        Cout << "   " << s << Endl;
    }

    Cout << "Method: " << req.Method << Endl
         << "Proto: " << req.Proto << Endl
         << "Headers:" << Endl;

    while (TStringBuf header = requestData.NextTok('\n')) {
        header.ChopSuffix("\r");
        if (header.empty()) {
            break;
        }

        Cout << "   " << header << Endl;
    }

    while (requestData.ChopSuffix("\r\n")) {
    }

    if (requestData.empty()) {
        Cout << "Body: (empty)" << Endl;
    } else {
        Cout << "Body: " << requestData.size() << Endl
             << requestData << Endl;
    }

    return EXIT_SUCCESS;
}
