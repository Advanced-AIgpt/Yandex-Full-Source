#include <alice/bass/libs/smallgeo/region.h>

#include <kernel/geodb/geodb.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/strip.h>

#include <iostream>
using namespace std;

using namespace NBASS::NSmallGeo;

namespace {

void ReadRegions(TStringBuf path, int root, TVector<TRegion>& regions) {
    NGeobase::TLookup lookup(static_cast<std::string>(path));

    const auto ids = lookup.GetTree(root);
    for (const auto id : ids) {
        const auto region = lookup.GetRegionById(id);

        if (region.GetType() < 0)
            continue;

        const NGeobase::TLinguistics lings = lookup.GetLinguistics(id, "ru");

        regions.emplace_back(region, lings);
    }
}

void ReadRegions(TStringBuf path, TVector<TRegion>& regions) {
    constexpr int ROOT = 0;
    ReadRegions(path, ROOT, regions);
}

} // namespace

int main(int argc, char** argv) {
    TString geobasePath;
    TString regionsPath;

    auto opts = NLastGetopt::TOpts::Default();
    opts.AddLongOption("geobase").StoreResult(&geobasePath).RequiredArgument("GEOBASE").Required();
    opts.AddLongOption("regions").StoreResult(&regionsPath).RequiredArgument("REGIONS").Required();
    opts.AddHelpOption();
    NLastGetopt::TOptsParseResult r(&opts, argc, argv);

    Cerr << "Collecting regions..." << Endl;

    TVector<TRegion> regions;
    ReadRegions(geobasePath, regions);
    TRegions::Instance().SaveToFile(regionsPath, regions);
    return 0;
}
