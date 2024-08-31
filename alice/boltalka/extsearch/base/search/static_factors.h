#pragma once

#include <alice/boltalka/extsearch/base/util/memory_mode.h>

#include <util/generic/ptr.h>
#include <util/memory/blob.h>

class IFactorsInfo;

namespace NNlg {

class TStaticFactorsStorage : public TThrRefBase {
public:
    TStaticFactorsStorage(const TString& filename, const IFactorsInfo* factorsInfo, EMemoryMode memoryMode);

    const float* GetFactors(size_t docId) const {
        return Factors + docId * NumFactors;
    }
    float GetFactor(size_t docId, size_t factorId) const {
        return GetFactors(docId)[factorId];
    }
    size_t GetNumFactors() const {
        return NumFactors;
    }

private:
    const size_t NumFactors;
    TBlob FactorsData;
    const float* Factors;
};

using TStaticFactorsStoragePtr = TIntrusivePtr<TStaticFactorsStorage>;

}
