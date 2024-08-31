#include "word_nn.h"

#include <util/stream/file.h>
#include <util/ysaveload.h>

namespace NVins {
    void SaveWordNnClassToIndicesMapping(const TString& fileName, const THashMap<TString, TVector<ui32>>& mapping) {
        TFileOutput file(fileName);
        ::Save(&file, mapping);
    }
} // namespace NVins
