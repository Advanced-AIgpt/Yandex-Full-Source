#include <alice/library/app_navigation/bno_apps_trie.h>

#include <library/cpp/containers/comptrie/comptrie.h>
#include <library/cpp/getopt/opt.h>


int main(int argc, char** argv) {
    NLastGetopt::TOpts opts;
    opts.SetFreeArgsNum(1);
    opts.SetFreeArgTitle(0, "Directory with bno.trie");
    NLastGetopt::TOptsParseResult res(&opts, argc, argv);

    TBlob trieData = TBlob::FromFileContent(res.GetFreeArgs()[0] + "/bno.trie");
    NAlice::TBnoAppsTrie trie(trieData);

    Cout << "bno.trie loaded" << Endl;

    TString key;
    while (true) {
        Cout << "> ";
        key = Cin.ReadLine();
        if (key == "q")
            break;

        NAlice::TBnoApp app;
        if (trie.Find(key, &app)) {
            Cout << " gplay:  " << app.AndroidAppId << Endl
                 << " iphone: " << app.IPhoneAppId << Endl
                 << " ipad:   " << app.IPadAppId << Endl;
        } else {
            Cout << "<not found>" << Endl;
        }
    }

    return 0;
}
