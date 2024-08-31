#include "knn_index.h"
#include "util.h"
#include <library/cpp/hnsw/index/dense_vector_distance.h>

namespace NNlg {

TKnnIndex::TKnnIndex(const TString& indexPrefix, size_t dimension, EMemoryMode memoryMode)
    : TKnnIndex(GetBlobFromFile(indexPrefix + ".index", memoryMode),
                GetBlobFromFile(indexPrefix + ".vec", memoryMode),
                GetBlobFromFile(indexPrefix + ".ids", memoryMode),
                dimension)
{
}

TKnnIndex::TKnnIndex(const TBlob& indexBlob, const TBlob& vectorBlob, const TBlob& idsBlob, size_t dimension)
    : Dimension(dimension)
    , ZeroVector(Dimension * 2)
    , KnnSearcher(indexBlob, vectorBlob, dimension)
    , IdsData(idsBlob)
    , Ids(reinterpret_cast<const ui32*>(IdsData.Begin()))
{
}

TVector<TKnnIndex::TSearchResult> TKnnIndex::FindNearestVectors(const float* query,
                                                                size_t topSize,
                                                                size_t searchNeighborhoodSize, size_t distanceCalcLimit) const {

    using TDistance = NHnsw::TDotProduct<float>;
    auto bestMatches = KnnSearcher.GetNearestNeighbors<TDistance>(query, topSize, searchNeighborhoodSize, distanceCalcLimit);
    TVector<TSearchResult> result;
    result.reserve(bestMatches.size());
    for (const auto& match : bestMatches) {
        result.push_back({ match.Id, Ids[match.Id], match.Dist });
    }
    return result;
}

}

