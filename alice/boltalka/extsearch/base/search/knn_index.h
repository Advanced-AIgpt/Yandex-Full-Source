#pragma once

#include <alice/boltalka/extsearch/base/util/memory_mode.h>

#include <library/cpp/hnsw/index/dense_vector_index.h>

#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>

namespace NNlg {

class TKnnIndex : public TThrRefBase {
public:
    struct TSearchResult {
        ui32 VectorId;
        ui32 DocId;
        float Score;
    };

    TKnnIndex(const TString& indexPrefix, size_t dimension, EMemoryMode memoryMode);
    TKnnIndex(const TBlob& indexBlob, const TBlob& vectorBlob, const TBlob& idsBlob, size_t dimension);
    TVector<TSearchResult> FindNearestVectors(const float* query, size_t topSize, size_t searchNeighborhoodSize, size_t distanceCalcLimit) const;

    const float* GetVector(ui32 vectorId) const { return KnnSearcher.GetItem(vectorId); }
    const float* GetZeroVector() const { return ZeroVector.data(); }
    size_t GetDimension() const { return Dimension; }

private:
    const size_t Dimension;
    const TVector<float> ZeroVector;
    NHnsw::THnswDenseVectorIndex<float> KnnSearcher;
    TBlob IdsData;
    const ui32* Ids;
};

using TKnnIndexPtr = TIntrusivePtr<TKnnIndex>;

}
