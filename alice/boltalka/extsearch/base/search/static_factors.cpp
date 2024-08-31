#include "static_factors.h"
#include <alice/boltalka/extsearch/base/factors/factor_names.h>

namespace NNlg {

namespace {

static size_t CalcNumStaticFactors(const IFactorsInfo* factorsInfo) {
    size_t numStaticFactors = 0;
    for (size_t i = 0; i < factorsInfo->GetFactorCount(); ++i) {
        if (factorsInfo->IsStaticFactor(i)) {
            ++numStaticFactors;
        }
    }
    return numStaticFactors;
}

}

TStaticFactorsStorage::TStaticFactorsStorage(const TString& filename, const IFactorsInfo* factorsInfo, EMemoryMode memoryMode)
    : NumFactors(CalcNumStaticFactors(factorsInfo))
    , FactorsData(memoryMode == EMemoryMode::Locked ? TBlob::LockedFromFile(filename) : TBlob::PrechargedFromFile(filename))
    , Factors(reinterpret_cast<const float*>(FactorsData.AsCharPtr()))
{
    Y_VERIFY(FactorsData.Size() % (NumFactors * sizeof(float)) == 0);
}

}

