#include "geobase.h"

namespace NAlice::NHollywood {

void TGeobaseResource::LoadFromPath(const TFsPath& dirPath) {
    auto geobasePath = dirPath / "geodata6.bin";
    Y_ENSURE(geobasePath.Exists(), "File not found: " << geobasePath);

    const TString geobasePathArcStr = geobasePath;
    const std::string geobasePathStr{geobasePathArcStr.data(), geobasePathArcStr.size()};

    Y_ENSURE(!GeobaseLookup_);
    GeobaseLookup_.ConstructInPlace(geobasePathStr, NGeobase::TInitTraits{}.LockMemory(LockMemoryMappedFile_));
}

} // namespace NAlice::NHollywood
