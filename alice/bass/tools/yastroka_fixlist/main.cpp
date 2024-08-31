#include <library/cpp/containers/comptrie/comptrie.h>
#include <library/cpp/getopt/opt.h>
#include <mapreduce/yt/client/init.h>
#include <mapreduce/yt/interface/client.h>
#include <util/folder/path.h>


TString AsString(const NYT::TNode& node) {
    if (node.IsString())
        return node.AsString();
    return TString();
}

int main(int argc, const char** argv) {
    NYT::Initialize(argc, argv);

    TString ytpath;
    TString outDir;

    NLastGetopt::TOpts opts;
    opts.SetFreeArgsNum(0);
    opts.AddLongOption("ytpath")
            .Help("YT table path in format 'serverName://tableName'")
            .Required()
            .StoreResult(&ytpath);
    opts.AddLongOption("out-dir")
            .Help("Output directory")
            .Optional()
            .DefaultValue(".")
            .StoreResult(&outDir);

    NLastGetopt::TOptsParseResult res(&opts, argc, argv);

    TString serverName, tableName;
    try {
        Split(ytpath, ':', serverName, tableName);
    } catch (const yexception& e) {
        Cerr << "Invalid table path format" << Endl;
        return -1;
    }
    if (serverName.empty() || tableName.empty()) {
        Cerr << "Invalid <table-path> value" << Endl;
        return -1;
    }

    NYT::IClientPtr client = NYT::CreateClient(serverName);
    if (!client->Exists(tableName)) {
        Cerr << "table '" << tableName << "' does not exist" << Endl;
        return -2; // don't change this value, it's used in /sandbox/projects/sup/YaStrokaFixList/__init__.py
    }

    TFsPath dir(outDir);
    dir.MkDirs();

    struct TValue {
        TString NormName;
        TString Title;
        TString Url;
        bool Native;

        Y_SAVELOAD_DEFINE(NormName, Title, Url, Native);
    };

    TCompactTrieBuilder<char, ui64> offsets; // raw name -> offset in data file
    THashMultiMap<TString, TString> names;   // normalized name -> [ raw name ]
    THashMap<TString, TValue> values;        // normalized name -> value

    Cout << "Loading table: '" << tableName << "' from '" << serverName << "'" << Endl;
    try {
        auto reader = client->CreateTableReader<NYT::TNode>(tableName);
        ui32 numRows = 0;
        for (; reader->IsValid(); reader->Next(), ++numRows) {
            const NYT::TNode::TMapType& row = reader->GetRow().AsMap();

            TString rawName = row.find("key")->second.AsString();
            TString normName = row.find("title")->second.AsString();
            TString title = AsString(row.find("friendly_name")->second);
            TString url = AsString(row.find("url")->second);
            bool native = row.find("native")->second.AsBool();

            TValue v{normName, title, url, native};

            names.insert({normName, rawName});
            if (!values.contains(normName))
                values[normName] = v;

            if (numRows % 1000 == 0)
                Cout << numRows << Endl;
        }
    } catch (const yexception& e) {
        Cerr << "Error reading table: " << e.what() << Endl;
        return -1;
    }

    Cout << "apps size: " << values.size() << Endl;

    TFixedBufferFileOutput dataFile(outDir + "/windows_fixlist.data");
    ui64 offset = 0;
    TBuffer buf{10 * 1024};
    for (auto& pair : values) {
        buf.Reset();
        TBufferOutput b{buf};
        pair.second.Save(&b);
        dataFile.Write(buf.Data(), buf.Size());
        auto range = names.equal_range(pair.first);
        for (auto it = range.first; it != range.second; ++it) {
            offsets.Add(it->second, offset);
        }
        offset += buf.Size();
    }
    dataFile.Flush();
    Cout << "value: saved " << offset << " bytes" << Endl;

    TFixedBufferFileOutput offsetsFile(outDir + "/windows_fixlist.offsets");
    size_t sz = CompactTrieMakeFastLayout(offsetsFile, offsets, false);
    offsetsFile.Flush();
    Cout << "offsets: saved " << sz << " bytes" << Endl;

    return 0;
}

