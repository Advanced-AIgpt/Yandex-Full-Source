#include <alice/library/app_navigation/bno_apps_trie.h>

#include <library/cpp/containers/comptrie/comptrie.h>
#include <library/cpp/getopt/opt.h>
#include <library/cpp/uri/uri.h>
#include <util/folder/path.h>
#include <util/string/cast.h>
#include <library/cpp/cgiparam/cgiparam.h>


TString ExtractAppId(TStringBuf url, bool gplay) {
    if (url.empty())
        return TString();

    NUri::TUri uri;
    NUri::TState::EParsed ps = uri.Parse(url, NUri::TFeature::FeaturesDefault | NUri::TFeature::FeatureSchemeKnown);
    if (ps != NUri::TState::EParsed::ParsedOK) {
        Cerr << "Cant parse app url " << url << Endl;
        return TString();
    }

    if (gplay) {
        // https://play.google.com/store/apps/details?id=com.whatsapp
        TCgiParameters cgi(uri.GetField(NUri::TField::FieldQuery));
        return cgi.Get("id");
    }

    // https://itunes.apple.com/ru/app/telegram-messenger/id686449807?mt=8
    TStringBuf path(uri.GetField(NUri::TField::FieldPath));
    return ToString(path.RNextTok('/'));
}


int main(int argc, const char** argv) {
    NLastGetopt::TOpts opts;

    opts.SetFreeArgsNum(1);
    opts.SetFreeArgTitle(0, "docids2app.txt", "Path to docids2app.txt");

    TString outDir;
    opts.AddLongOption("out-dir")
            .Help("Output directory")
            .Optional()
            .DefaultValue(".")
            .StoreResult(&outDir);

    opts.AddCharOption('f', NLastGetopt::EHasArg::NO_ARGUMENT, "Collect only fully described apps");

    NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    TString inFile = res.GetFreeArgs()[0];
    bool onlyFullApps = res.Has('f');

    TFsPath dir(outDir);
    dir.MkDirs();

    TCompactTrieBuilder<char, NAlice::TBnoApp, NAlice::TBnoAppPacker> trie; // docid -> apps

    try {
        TFileInput fin(inFile);
        TString line;
        int numRows = 0, numDocs = 0;
        while (fin.ReadLine(line)) {
            if (++numRows % 1000 == 0)
                Cout << numRows << Endl;

            TStringBuf docId, androidUrl, iphoneUrl, ipadUrl;
            Split(line, '\t', docId, androidUrl, iphoneUrl, ipadUrl);
            if (!docId || docId == "#") {
                continue;
            }

            NAlice::TBnoApp app{
                    .AndroidAppId = ExtractAppId(androidUrl, true /* gplay */),
                    .IPhoneAppId = ExtractAppId(iphoneUrl, false /* gplay */),
                    .IPadAppId = ExtractAppId(ipadUrl, false /* gplay */)
            };

            if (onlyFullApps) {
                if (!app.AndroidAppId || !app.IPhoneAppId || !app.IPadAppId)
                    continue;
            }

            NAlice::TBnoApp existing;
            if (trie.Find(docId, &existing)) {
                if (app == existing) {
                    continue;
                }
                if (!app.MergeWith(existing)) {
                   throw yexception() << docId << ": " << existing << " != " << app;
                } else {
                    Cout << "merge " << docId << Endl;
                }
            }

            if (trie.Add(docId, app)) {
                ++numDocs;
            }
        }

        Cout << "total rows: " << numRows << Endl;
        Cout << "total docs: " << numDocs << Endl;
    } catch (const yexception& e) {
        Cerr << "Error reading file: " << e.what() << Endl;
        return -1;
    }

    TFixedBufferFileOutput bnoFile(outDir + "/bno.trie");
    size_t sz = CompactTrieMakeFastLayout(bnoFile, trie, false);
    bnoFile.Flush();
    Cout << "saved " << sz << " bytes, " << trie.GetEntryCount() << " entries" << Endl;

    return 0;
}
