#include <alice/bass/libs/smallgeo/result.h>

#include <kernel/geodb/geodb.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/resource/resource.h>

#include <iomanip>
#include <iostream>
using namespace std;

using namespace NBASS::NSmallGeo;

void Panic(TStringBuf message) {
    Cerr << TString(message) << Endl;
    exit(-1);
}

bool NotSpace(TString& s, int i) {
    TString t = " -";
    return t[0] != s[i] && t[1] != s[i];
}

// Print pairs of nested regions with similar names
void PrintRegions(TVector<TRegion>& regions) {
    TVector<TString> names(regions.size());
    THashMap<int, int> indexById;

    for (size_t i = 0; i < regions.size(); ++i) {
        names[i] = regions[i].Name;
        indexById[regions[i].Id] = i;
    }

    for (auto reg: regions) {
        if (!reg.ParentId) {
            continue;
        }
        if (reg.ParentId < 0) {
            Cerr << "ERROR\n";
            continue;
        }
        TString nameChild = names[indexById[reg.Id]];
        TString namePar = names[indexById[reg.ParentId]];
        size_t len = std::min(nameChild.size(), namePar.size());
        if (nameChild.substr(0, len) == namePar.substr(0, len)) {
            if (nameChild.size() > len && NotSpace(nameChild, len)) {
                continue;
            }
            if (namePar.size() > len && NotSpace(namePar, len)) {
                continue;
            }
            // Region must be not bigger than a city, parent - smaller than a city
            if (reg.Type > NGeoDB::CITY || regions[indexById[reg.ParentId]].Type > NGeoDB::CONSTITUENT_ENTITY) {
                continue;
            }
            // Islands
            if (reg.Type == NGeoDB::REGION || regions[indexById[reg.ParentId]].Type == NGeoDB::REGION) {
                continue;
            }
            cout << namePar << " (" << reg.ParentId << ", " << regions[indexById[reg.ParentId]].Type << ")" <<
                " --- " << nameChild << " (" << reg.Id << ", " << regions[indexById[reg.Id]].Type << ") " << endl;
        }
    }
    cout << endl;
}


int main() {
    cout << fixed << setprecision(3);

    TVector<TRegion> regions;
    {
        TString data;
        if (!NResource::FindExact("regions.data", &data))
            Panic("Can't find regions resource");
        TStringInput input(data);
        regions = TRegions::Instance().LoadFromStream(input);
    }

    PrintRegions(regions);
    return 0;
}
