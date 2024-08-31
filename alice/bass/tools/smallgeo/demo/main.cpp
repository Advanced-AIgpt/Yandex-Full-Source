#include <alice/bass/libs/smallgeo/engine.h>
#include <alice/bass/libs/smallgeo/latlon.h>
#include <alice/bass/libs/smallgeo/result.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/resource/resource.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/string/builder.h>
#include <util/system/yassert.h>

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
using namespace std;

using namespace NBASS::NSmallGeo;

namespace {
TString FLAG_POSITION;
int FLAG_TOP = 5;
bool FLAG_VERBOSE = false;
bool FLAG_ONLY_TOP_ID = false;

void Panic(TStringBuf message) {
    cerr << TString(message) << endl;
    exit(-1);
}

TMaybe<TLatLon> ParsePosition(const TString& value) {
    if (value.empty())
        return Nothing();

    if (value == "yandex")
        return TLatLon{55.734380, 37.587085};

    TVector<double> coords;
    try {
        Split(value, ";", [&](TStringBuf part) { coords.emplace_back(FromString<double>(part)); });
    } catch (const TFromStringException& e) {
        Panic(e.what());
    }

    if (coords.size() != 2)
        Panic("There should be exactly two coords");

    const auto lat = coords[0];
    const auto lon = coords[1];

    if (lat < TLatLon::MIN_LAT || lat > TLatLon::MAX_LAT)
        Panic(TStringBuilder() << "Invalid LAT value: " << lat);
    if (lon < TLatLon::MIN_LON || lon > TLatLon::MAX_LON)
        Panic(TStringBuilder() << "Invalid LON value: " << lon);

    return TLatLon{lat, lon};
}

void ProcessQuery(TStringBuf query, const TMaybe<TLatLon>& position, const TVector<TRegion>& regions,
                  const TEngine& engine) {
    if (FLAG_ONLY_TOP_ID) {
        const auto results = engine.FindTopRegions(query, position, 1 /* top */);
        Y_ASSERT(results.size() <= 1);
        if (results.empty())
            cout << "-1" << endl;
        else
            cout << regions[results[0].Index].Id << endl;
        return;
    }

    const auto results = engine.FindTopRegions(query, position, FLAG_TOP);

    for (const auto& result : results) {
        const auto& region = regions[result.Index];
        const double sim = result.Similarity / static_cast<double>(TResult::MAX_SIMILARITY);
        cout << engine.GetExtendedName(result.Index) << ", id: " << region.Id << ", sim: " << sim << ", type: " << result.Type;
        if (result.DistanceM) {
            const double dist = *result.DistanceM / 1000.0;
            cout << ", dist: " << dist << "km";
        }

        cout << endl;
    }
}
} // namespace

int main(int argc, char** argv) {
    auto opts = NLastGetopt::TOpts::Default();
    opts.AddLongOption("position")
        .StoreResult(&FLAG_POSITION)
        .RequiredArgument("POSITION")
        .Help("When this flag is set, distance factor will be used in addition to other factors to rank results. "
              "Flag value may be a pre-defined shortcut from the following list: ['yandex']. Or it may represent raw "
              "coords in the following format: LAT;LON.");
    opts.AddLongOption("top").StoreResult(&FLAG_TOP).RequiredArgument("TOP").DefaultValue(5);
    opts.AddLongOption("verbose").StoreResult(&FLAG_VERBOSE).RequiredArgument("VERBOSE").DefaultValue("no");
    opts.AddLongOption("only-top-id")
        .StoreResult(&FLAG_ONLY_TOP_ID)
        .RequiredArgument("ONLY-TOP-ID")
        .DefaultValue("no");
    opts.AddHelpOption();
    NLastGetopt::TOptsParseResult r(&opts, argc, argv);

    const auto position = ParsePosition(FLAG_POSITION);

    cout << fixed << setprecision(3);

    TVector<TRegion> regions;
    {
        TString data;
        if (!NResource::FindExact("regions.data", &data))
            Panic("Can't find regions resource");
        TStringInput input(data);
        regions = TRegions::Instance().LoadFromStream(input);
    }


    TEngine engine(regions, FLAG_VERBOSE);

    uint64_t processed = 0;
    string query;
    while (true) {
        if (!getline(cin, query))
            break;

        ProcessQuery(query, position, regions, engine);

        ++processed;
        if (FLAG_VERBOSE && processed % 1000 == 0)
            cerr << endl << "Processed " << processed << " queries" << endl;
    }

    return 0;
}
