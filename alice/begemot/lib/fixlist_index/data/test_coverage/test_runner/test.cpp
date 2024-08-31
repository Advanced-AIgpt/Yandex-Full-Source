#include <alice/begemot/lib/fixlist_index/fixlist_index.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/testing/common/env.h>

#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/folder/path.h>
#include <util/stream/file.h>
#include <util/string/split.h>

#include <iostream>

constexpr TStringBuf FILE_SUFFIX = ".yaml";
constexpr TStringBuf GENERAL_FIXLIST = "general_fixlist";
constexpr TStringBuf GC_REQUEST_BANLIST = "gc_request_banlist";

THashSet<TString> ApplyFixList(TFsPath dataPath, TFsPath fixlistPath, TStringBuf fixlistType) {
    THashSet<TString> matched;

    TFileInput dataFileStream(dataPath);
    TFileInput fixlistFileStream(fixlistPath);

    NBg::TFixlistIndex index;
    index.AddFixlist(fixlistType, &fixlistFileStream);

    TString line;
    bool firstLine = true;
    while (dataFileStream.ReadLine(line)) {
        if (firstLine) {
            firstLine = false;
            continue;
        }
        auto s = StringSplitter(line).Split('\t').Take(2);
        TVector<TString> v{s.begin(), s.end()};
        TString text = v[1];
        
        NBg::TFixlistIndex::TQuery query;
        query.Query = text;
        if (!index.MatchAgainst(query, fixlistType).empty()) {
            matched.insert(text);
        }
    }
    return matched;
}

TFsPath GetFixListDataFolder() {
    return ArcadiaSourceRoot() + TStringBuf("/alice/begemot/lib/fixlist_index/data/ru");
}
TFsPath GetRequestsDataFolder() {
    return BinaryPath("alice/begemot/lib/fixlist_index/data/test_coverage/pool");
}

TFsPath GetTargetPath(TFsPath dataPath, TStringBuf fixlistType) {
    TFsPath target = ToString(fixlistType) + TString("_") + dataPath.Basename();
    return ArcadiaSourceRoot() + TStringBuf("/alice/begemot/lib/fixlist_index/data/test_coverage/target") / target;
}

THashSet<TString> ReadTarget(TFsPath targetPath) {
    THashSet<TString> result;
    TFileInput fileStream(targetPath);
    TString line;
    while (fileStream.ReadLine(line)) {
        result.insert(line);
    }
    return result;
}

bool Test(TFsPath dataPath, TFsPath fixlistPath, TStringBuf fixlistType, int outputLimit = 50) {
    std::cout << "Fixlist Type: " << fixlistType << std::endl;
    const auto targetData = ReadTarget(GetTargetPath(dataPath, fixlistType));

    const auto realData = ApplyFixList(dataPath, fixlistPath, fixlistType);

    THashSet<TString> newCoverage;
    THashSet<TString> lostCoverage;

    for (const auto& rd : realData) {
        if (!targetData.contains(rd)) {
            newCoverage.insert(rd);
        }
    }

    for (const auto& td : targetData) {
        if (!realData.contains(td)) {
            lostCoverage.insert(td);
        }
    }

    if (newCoverage.empty() && lostCoverage.empty()) {
        return true;
    }

    int cnt = 0;
    std::cout << "New:" << std::endl;
    for (const TString& newReq : newCoverage) {
        std::cout << newReq << std::endl;
        if (cnt++ == outputLimit) {
            break;
        }
    }
    cnt = 0;
    std::cout << "Lost:" << std::endl;
    for (const TString& lostReq : lostCoverage) {
        std::cout << lostReq << std::endl;
        if (cnt++ == outputLimit) {
            break;
        }
    }
    return false;
}

void Canonize(TFsPath dataPath, TFsPath fixlistPath, TStringBuf fixlistType) {
    const auto realData = ApplyFixList(dataPath, fixlistPath, fixlistType);
    TFileOutput fileStream(GetTargetPath(dataPath, fixlistType));
    TVector<TString> sortedData(realData.begin(), realData.end());
    std::sort(sortedData.begin(), sortedData.end());
    for (const TString& request : sortedData) {
        fileStream << request << '\n';
    }
}

int main(int argc, const char** argv) {
    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
    TFsPath fileName;
    bool canonize = true;
    opts.AddLongOption("data", "Requests file name.")
        .Required()
        .RequiredArgument("INPUT")
        .StoreResult(&fileName);
    opts.AddLongOption("canonize", "Canonize fixlixt results.").StoreTrue(&canonize);
    NLastGetopt::TOptsParseResult config(&opts, argc, argv);

    TFsPath gcRequestBanlistPath = GetFixListDataFolder() / (ToString(GC_REQUEST_BANLIST) + ToString(FILE_SUFFIX));
    TFsPath generalFixlistPath = GetFixListDataFolder() / (ToString(GENERAL_FIXLIST) + ToString(FILE_SUFFIX));

    if (canonize) {
        Canonize(GetRequestsDataFolder() / fileName, gcRequestBanlistPath, GC_REQUEST_BANLIST);
        Canonize(GetRequestsDataFolder() / fileName, generalFixlistPath, GENERAL_FIXLIST);
    } else {
        if (Test(GetRequestsDataFolder() / fileName, gcRequestBanlistPath, GC_REQUEST_BANLIST) && 
            Test(GetRequestsDataFolder() / fileName, generalFixlistPath, GENERAL_FIXLIST)) {
                return 0;
        }
        return 1;
    }
}
