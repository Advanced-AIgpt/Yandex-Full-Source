#include <library/cpp/containers/comptrie/comptrie.h>
#include <library/cpp/getopt/opt.h>

#include <util/string/printf.h>


struct TValue {
    TString NormName;
    TString Title;
    TString Url;
    bool Native;

    Y_SAVELOAD_DEFINE(NormName, Title, Url, Native);
};

int main(int argc, char** argv) {
    TString dir;

    NLastGetopt::TOpts opts;
    opts.SetFreeArgsNum(0);
    opts.AddLongOption("dir")
            .Help("Directory with fixlist files")
            .Optional()
            .DefaultValue(".")
            .StoreResult(&dir);

    NLastGetopt::TOptsParseResult res(&opts, argc, argv);

    TBlob offsetsData = TBlob::FromFileContent(Sprintf("%s/windows_fixlist.offsets", dir.data()));
    TCompactTrie<char, ui64> offsets(offsetsData);

    TBlob data = TBlob::FromFileContent(Sprintf("%s/windows_fixlist.data", dir.data()));

    Cout << "fixlist loaded" << Endl;

    TString key;
    while (true) {
        Cout << "> ";
        key = Cin.ReadLine();
        if (key == "q")
            break;

        ui64 offset = 0;
        if (offsets.Find(key, &offset)) {
            TValue v;
            TBlob subBlob = data.SubBlob(offset, data.Length());
            TMemoryInput in{subBlob.Data(), subBlob.Length()};
            v.Load(&in);
            Cout << " NormName: " << v.NormName << Endl
                 << " Title: " << v.Title << Endl
                 << " Url: " << v.Url << Endl
                 << " Native: " << v.Native << Endl
                 << " __offset__: " << offset << Endl;
        } else {
            Cout << "<not found>" << Endl;
        }
    }

    return 0;
}
