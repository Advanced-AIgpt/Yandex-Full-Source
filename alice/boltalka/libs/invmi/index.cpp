#include "index.h"
#include "math_ops.h"

#include <library/cpp/containers/limited_heap/limited_heap.h>
#include <library/cpp/containers/top_keeper/top_keeper.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/generic/queue.h>
#include <util/generic/buffer.h>
#include <util/stream/file.h>
#include <util/generic/hash_set.h>

TInvMiModel::TInvMiModel(const TString& filename, size_t dimension)
    : Dimension(dimension)
{
    LoadFromBlob(TBlob::PrechargedFromFile(filename));
}

TInvMiModel::TInvMiModel(const TBlob& blob, size_t dimension)
    : Dimension(dimension)
{
    LoadFromBlob(blob);
}

void TInvMiModel::LoadFromBlob(const TBlob& blob) {
    Y_VERIFY(Dimension % 2 == 0);

    ModelData = blob;
    const unsigned char* pos = ModelData.AsUnsignedCharPtr();

    K = *reinterpret_cast<const int32_t*>(pos);
    pos += sizeof(int32_t);

    RotationMatrix = reinterpret_cast<const float*>(pos);
    pos += Dimension * Dimension * sizeof(float);

    Centroids1 = reinterpret_cast<const float*>(pos);
    pos += K * Dimension / 2 * sizeof(float);
    Centroids2 = reinterpret_cast<const float*>(pos);
    pos += K * Dimension / 2 * sizeof(float);

    ModelDataEnd = pos;

    CentroidNorms1.resize(K);
    CentroidNorms2.resize(K);
    for (size_t k = 0; k < K; ++k) {
        size_t offset = k * Dimension / 2;
        const float* centroid1 = Centroids1 + offset;
        CentroidNorms1[k] = SqrL2Norm(centroid1, Dimension / 2);
        const float* centroid2 = Centroids2 + offset;
        CentroidNorms2[k] = SqrL2Norm(centroid2, Dimension / 2);
    }
}

size_t TInvMiModel::GetClusterId(size_t centroidId1, size_t centroidId2) const {
    return centroidId1 * K + centroidId2;
}

size_t TInvMiModel::GetCentroidId1(size_t clusterId) const {
    return clusterId / K;
}

size_t TInvMiModel::GetCentroidId2(size_t clusterId) const {
    return clusterId % K;
}

size_t TInvMiModel::GetNumClusters() const {
    return K * K;
}


TInvMiIndex::TInvMiIndex(const TString& invMiModelFilename, size_t dimension)
    : TInvMiModel(invMiModelFilename, dimension)
{
    TryLoadClustersFromInvMiModel();
}

TInvMiIndex::TInvMiIndex(const TBlob& invMiModelBlob, size_t dimension)
    : TInvMiModel(invMiModelBlob, dimension)
{
    TryLoadClustersFromInvMiModel();
}

TConstArrayRef<ui32> TInvMiIndex::GetCluster(size_t clusterId) const {
    size_t clusterBegin = clusterId ? CumClusterSizes[clusterId - 1] : 0;
    size_t clusterEnd = CumClusterSizes[clusterId];
    return { VectorIds + clusterBegin, VectorIds + clusterEnd };
}

void TInvMiIndex::TryLoadClustersFromInvMiModel() {
    const size_t invMiModelSize = ModelDataEnd - ModelData.AsUnsignedCharPtr();
    TBlob clusterBlob = ModelData.SubBlob(invMiModelSize, ModelData.Size());
    if (clusterBlob.Empty()) {
        return;
    }
    const size_t numClusters = GetNumClusters();
    NumVectors = clusterBlob.Size() / sizeof(ui32) - numClusters;

    const unsigned char* pos = clusterBlob.AsUnsignedCharPtr();
    VectorIds = reinterpret_cast<const ui32*>(pos);
    pos += NumVectors * sizeof(ui32);
    CumClusterSizes = reinterpret_cast<const ui32*>(pos);
    pos += numClusters * sizeof(ui32);
    Y_VERIFY(pos == clusterBlob.End());
    InitNonEmptyCluster();
}

void TInvMiIndex::InitNonEmptyCluster() {
    NonEmpty.clear();
    for (ui32 i = 0; i < K; ++i) {
        for (ui32 j = 0; j < K; ++j) {
            size_t c = i * K + j;
            size_t size = CumClusterSizes[c];
            if (c) size -= CumClusterSizes[c - 1];
            if (size > 0) {
                NonEmpty.push_back({ i, j, GetCluster(c) });
            }
        }
    }
}

float TInvMiIndex::CalcAverageClusterSize() const {
    return NumVectors / static_cast<float>(GetNumClusters());
}

void TInvMiIndex::SerializeAsInvMiModel(const TString& filename) const {
    TFixedBufferFileOutput out(filename);
    out.Write(ModelData.Begin(), ModelDataEnd - ModelData.Begin());
    out.Write(VectorIds, NumVectors * sizeof(ui32));
    out.Write(CumClusterSizes, GetNumClusters() * sizeof(ui32));
}

static TVector<ui32> AssignToCentroids(size_t dimension,
                                       const float* centroids, const float* centroidNorms, size_t numCentroids,
                                       const float* rotatedVectors, size_t numVectors) {

    static const size_t blockSize = 10000;
    TVector<ui32> assignments;
    assignments.reserve(numVectors);

    size_t vectorsProcessed = 0;
    TVector<float> distToCentroids(blockSize * numCentroids);
    while (vectorsProcessed < numVectors) {
        size_t curBlockSize = Min(blockSize, numVectors - vectorsProcessed);
        MatrixMatrixMultiply({ rotatedVectors, curBlockSize, dimension / 2, CblasNoTrans, -2.0, dimension },
                             { centroids, numCentroids, dimension / 2, CblasTrans },
                             { distToCentroids.data(), curBlockSize, numCentroids });

        for (size_t i = 0; i < curBlockSize; ++i) {
            float* dists = distToCentroids.data() + i * numCentroids;
            AddScaledVector(dists, 1.0, centroidNorms, numCentroids);
            assignments.push_back(std::min_element(dists, dists + numCentroids) - dists);
        }
        vectorsProcessed += curBlockSize;
        rotatedVectors += dimension * curBlockSize;
    }
    Y_VERIFY(assignments.size() == numVectors);
    return assignments;
}

void TInvMiIndex::BuildIndex(const TBlob& vectorsBlob) {
    VectorData = vectorsBlob;

    Vectors = reinterpret_cast<const float*>(VectorData.AsUnsignedCharPtr());
    NumVectors = VectorData.Size() / sizeof(float) / Dimension;

    TVector<float> rotatedVectors(NumVectors * Dimension);
    Cerr << "Rotating..." << Endl;
    MatrixMatrixMultiply({ Vectors, NumVectors, Dimension, CblasNoTrans, 1.0 },
                         { RotationMatrix, Dimension, Dimension, CblasTrans },
                         { rotatedVectors.data(), NumVectors, Dimension });

    const float* halfRotatedVectors = rotatedVectors.data();
    Cerr << "assigning 1..." << Endl;
    TVector<ui32> assignments1 = AssignToCentroids(Dimension,
                                                   Centroids1, CentroidNorms1.data(), K,
                                                   halfRotatedVectors, NumVectors);
    halfRotatedVectors += Dimension / 2;
    Cerr << "assigning 2..." << Endl;
    TVector<ui32> assignments2 = AssignToCentroids(Dimension,
                                                   Centroids2, CentroidNorms2.data(), K,
                                                   halfRotatedVectors, NumVectors);
    TVector<ui32> assignments(NumVectors);
    for (size_t i = 0; i < NumVectors; ++i) {
        assignments[i] = GetClusterId(assignments1[i], assignments2[i]);
    }

    Cerr << "sorting..." << Endl;
    VectorIdsData.resize(NumVectors);
    for (size_t i = 0; i < NumVectors; ++i) {
        VectorIdsData[i] = i;
    }
    std::sort(VectorIdsData.begin(), VectorIdsData.end(),
        [&assignments](ui32 i, ui32 j) {
            return std::tie(assignments[i], i) < std::tie(assignments[j], j);
        }
    );
    VectorIds = VectorIdsData.data();

    Cerr << "clustering..." << Endl;
    std::sort(assignments.begin(), assignments.end());
    size_t numClusters = GetNumClusters();
    CumClusterSizesData.clear();
    CumClusterSizesData.resize(numClusters);
    ui32 sumSizes = 0;
    for (size_t i = 0; i < numClusters; ++i) {
        while (sumSizes < NumVectors && assignments[sumSizes] == i) {
            ++sumSizes;
        }
        CumClusterSizesData[i] = sumSizes;
    }
    CumClusterSizes = CumClusterSizesData.data();
    Y_VERIFY(sumSizes == NumVectors);
    InitNonEmptyCluster();

    Cerr << "error..." << Endl;
    double error = 0.0;
    TVector<float> reconstruction(Dimension);
    for (size_t i = 0; i < NumVectors; ++i) {
        size_t c1 = assignments1[i];
        size_t c2 = assignments2[i];
        std::copy(Centroids1 + c1  * Dimension / 2, Centroids1 + (c1 + 1) * Dimension / 2, reconstruction.data());
        std::copy(Centroids2 + c2  * Dimension / 2, Centroids2 + (c2 + 1) * Dimension / 2, reconstruction.data() + Dimension / 2);
        AddScaledVector(reconstruction.data(), -1.0, rotatedVectors.data() + i * Dimension, Dimension);
        error += SqrL2Norm(reconstruction.data(), Dimension);
    }
    Cerr << "Error: " << error / NumVectors << Endl;
}

void TInvMiIndex::BuildIndex(const TString& vectorsFilename) {
    BuildIndex(TBlob::PrechargedFromFile(vectorsFilename));
}

void TInvMiIndex::LinkToVectors(const TBlob& vectorsBlob) {
    VectorData = vectorsBlob;
    Vectors = reinterpret_cast<const float*>(VectorData.AsUnsignedCharPtr());
    NumVectors = VectorData.Size() / sizeof(float) / Dimension;
}

void TInvMiIndex::LinkToVectors(const TString& vectorsFilename) {
    LinkToVectors(TBlob::PrechargedFromFile(vectorsFilename));
}

void TInvMiIndex::ConvertToByteVectors() {
    AlignedDimension = (Dimension * sizeof(i8) + 15) / 16 * 16;
    TBuffer buffer;
    TVector<char> byteVector(AlignedDimension, 0);
    for (size_t vectorId = 0; vectorId < NumVectors; ++vectorId) {
        const float* data = Vectors + vectorId * Dimension;
        for (size_t i = 0; i < Dimension; ++i) {
            byteVector[i] = static_cast<char>(data[i]);
        }
        buffer.Append(byteVector.data(), byteVector.size());
    }
    VectorData = TBlob::FromBuffer(buffer);
    Vectors = nullptr;
    ByteVectors = reinterpret_cast<const i8*>(VectorData.AsCharPtr());
}

void TInvMiIndex::TSearchThreadData::Init(size_t K, size_t /*numVectors*/, size_t dimension) {
    RotatedQuery.resize(dimension);
    Dist1.resize(K);
    Dist2.resize(K);
    DistToId1.resize(K);
    DistToId2.resize(K);
    NumVisitedColumns.assign(K, 0);
    NearestClusterIds.clear();
    NearestClusterIds.reserve(K * K);
    FoundClusters.clear();
    FoundClusters.reserve(K * K);
}

TInvMiIndex::TSearcher::TSearcher(const TInvMiIndex* index)
    : Index(index)
{
}


TVector<TInvMiIndex::TSearchResult> TInvMiIndex::TSearcher::GetNearestVectors(const float* query,
                                                                              size_t topSize,
                                                                              size_t minClustersToSearch) {
    return Index->GetNearestVectors(&Data, query, topSize, minClustersToSearch);
}

TInvMiIndex::TSearcher TInvMiIndex::CreateSearcher() const {
    return TSearcher(this);
}

static void CalcDistToCentroids(size_t dimension,
                                const float* centroids, const float* centroidNorms, size_t numCentroids,
                                const float* rotatedQuery,
                                float* dist) {

    MatrixMatrixMultiply({ centroids, numCentroids, dimension / 2, CblasNoTrans, -2.0 },
                         { rotatedQuery, dimension / 2, 1 },
                         { dist, numCentroids, 1 });
    AddScaledVector(dist, 1.0, centroidNorms, numCentroids);

    float queryNorm = SqrL2Norm(rotatedQuery, dimension / 2);
    for (size_t i = 0; i < numCentroids; ++i) {
        dist[i] += queryNorm;
    }
}

void TInvMiIndex::GetNearestClusterIds(TSearchThreadData* ctx,
                                       size_t minVectorsToSearch,
                                       size_t minClustersToSearch) const {

    /*for (size_t i = 0; i < minClustersToSearch; ++i) {
        ctx->FoundClusters.push_back(NonEmpty[i].Vectors);
    }
    return;*/
    /*using TNode = std::pair<float, TClusterInfo>;
    TLimitedHeap<TNode, TLess<TNode>> top(minClustersToSearch, TLess<TNode>());
    //TTopKeeper<TNode, TLess<TNode>> top(minClustersToSearch);
    for (const auto& cluster : NonEmpty) {
        float dist = ctx->Dist1[cluster.X] + ctx->Dist2[cluster.Y];
        top.Insert(std::make_pair(dist, cluster));
    }
    for (; !top.IsEmpty(); top.PopMin()) {
        ctx->FoundClusters.push_back(top.GetMin().second.Vectors);
    }
    std::reverse(ctx->FoundClusters.begin(), ctx->FoundClusters.end()); */
    /*
    const auto& internal = top.GetInternal();
    for (size_t i = 0; i < Min(minClustersToSearch, internal.size()); ++i) {
        ctx->FoundClusters.push_back(internal[i].second.Vectors);
    }*/
    //return;
    for(ui32 k = 0; k < K; ++k) {
        ctx->DistToId1[k] = { ctx->Dist1[k], k };
        ctx->DistToId2[k] = { ctx->Dist2[k], k };
    }
    /*size_t approxTop = 300;
    //static TVector<std::pair<float, size_t>> clusters;
    //clusters.clear();
    //clusters.reserve(approxTop * approxTop);
    std::partial_sort(ctx->DistToId1.begin(), ctx->DistToId1.begin() + approxTop, ctx->DistToId1.end());
    std::partial_sort(ctx->DistToId2.begin(), ctx->DistToId2.begin() + approxTop, ctx->DistToId2.end());
    for (size_t sum = 0; sum < 2 * approxTop; ++sum) {
        for (size_t y = 0; y <= sum; ++y) {
            size_t x = sum - y;
            //float dist = ctx->DistToId1[i].first + ctx->DistToId2[j].first;
            size_t clusterId = GetClusterId(ctx->DistToId1[x].second, ctx->DistToId2[y].second);
            ctx->NearestClusterIds.push_back(clusterId);
            //clusters.push_back(std::make_pair(dist, clusterId));
        }
    }
    //std::sort(clusters.begin(), clusters.end());
    //for (const auto& pair : clusters) {
    //    ctx->NearestClusterIds.push_back(pair.second);
    //}
    return;*/

    std::sort(ctx->DistToId1.begin(), ctx->DistToId1.end());
    std::sort(ctx->DistToId2.begin(), ctx->DistToId2.end());

    TPriorityQueue<std::pair<float, size_t>, TVector<std::pair<float, size_t>>, TGreater<std::pair<float, size_t>>> heap;
    float dist = ctx->DistToId1.front().first + ctx->DistToId2.front().first;
    size_t clusterPos = GetClusterId(0, 0);
    heap.emplace(dist, clusterPos);

    size_t numVisitedVectors = 0;
    while (!heap.empty() && (ctx->NearestClusterIds.size() < minClustersToSearch || numVisitedVectors < minVectorsToSearch)) {
    //while (!heap.empty() && (ctx->FoundClusters.size() < minClustersToSearch || numVisitedVectors < minVectorsToSearch)) {
        size_t clusterPos = heap.top().second;
        heap.pop();

        //ctx->IsVisitedCluster[clusterPos] = isVisited;
        ui32 ptr1 = GetCentroidId1(clusterPos);
        //size_t ptr2 = GetCentroidId2(clusterPos);
        ui32 ptr2 = clusterPos - ptr1 * K;
        ++ctx->NumVisitedColumns[ptr1];

        size_t clusterId = GetClusterId(ctx->DistToId1[ptr1].second, ctx->DistToId2[ptr2].second);
        /*size_t clusterBegin = clusterId ? CumClusterSizes[clusterId - 1] : 0;
        size_t clusterEnd = CumClusterSizes[clusterId];
        if (clusterBegin != clusterEnd) {
            ctx->FoundClusters.push_back({ VectorIds + clusterBegin, VectorIds + clusterEnd });
            numVisitedVectors += clusterEnd - clusterBegin;
        }*/
        ctx->NearestClusterIds.push_back(clusterId);
        numVisitedVectors = minVectorsToSearch;
        /*if (ctx->NearestClusterIds.size() == 50000) {
            break;
        }*/
        //if (numVisitedVectors < minVectorsToSearch) {
        //numVisitedVectors += CumClusterSizes[clusterId] - (clusterId ? CumClusterSizes[clusterId - 1] : 0);
        //}

        /*
        if (ptr2 + 1 < K) {
            heap.emplace(ctx->DistToId1[ptr1].first + ctx->DistToId2[ptr2 + 1].first, ptr1 * K + ptr2 + 1);
        }
        if (ptr2 == 0 && ptr1 + 1 < K) {
            heap.emplace(ctx->DistToId1[ptr1 + 1].first + ctx->DistToId2[ptr2].first, (ptr1 + 1) * K + ptr2);
        }
        //*/

        //*
        if(ptr2 + 1 < K && (!ptr1 || ctx->NumVisitedColumns[ptr1 - 1] > ptr2 + 1)) {
            heap.emplace(ctx->DistToId1[ptr1].first + ctx->DistToId2[ptr2 + 1].first, ptr1 * K + ptr2 + 1);
        }
        if(ptr1 + 1 < K && (!ptr2 || ctx->NumVisitedColumns[ptr1 + 1] == ptr2)) {
            heap.emplace(ctx->DistToId1[ptr1 + 1].first + ctx->DistToId2[ptr2].first, (ptr1 + 1) * K + ptr2);
        }
        //*/
    }
    for (size_t clusterId : ctx->NearestClusterIds) {
        auto cluster = GetCluster(clusterId);
        if (!cluster.empty()) {
            ctx->FoundClusters.push_back(cluster);
        }
    }
    //Cerr << minClustersToSearch << "\t" << numVisitedVectors << '\t' << heap.size() << Endl;
}

template<typename TFeatureType>
static TVector<TInvMiIndex::TSearchResult> SelectNearestFromClusters(size_t dimension,
                                                                     size_t alignedDimension,
                                                                     const TFeatureType* vectors,
                                                                     const TFeatureType* query,
                                                                     const TVector<TConstArrayRef<ui32>> &clusters,
                                                                     size_t topSize)
{
    TLimitedHeap<TInvMiIndex::TSearchResult, TLess<TInvMiIndex::TSearchResult>> top(topSize);
    size_t numVectors = 0;
    for (const auto& cluster : clusters) {
        //ui32 i = *cluster.begin();
        for (ui32 vectorId : cluster) {
        //for (size_t j = 0; j < cluster.size(); ++j) {
            ++numVectors;
            /*float dist = DotProduct(query, vectors + i++ * alignedDimension, dimension);
            if (top.GetSize() < topSize || dist > top.GetMin().Dist) {
                top.Insert({ dist, cluster[j] });
            }*/
            float dist = DotProduct(query, vectors + vectorId * alignedDimension, dimension);
            top.Insert({ dist, vectorId });
        }
        //if (numVectors >= 30000) break;
    }
    TVector<TInvMiIndex::TSearchResult> result;
    result.reserve(top.GetSize());
    for (; !top.IsEmpty(); top.PopMin()) {
        result.push_back(top.GetMin());
    }
    std::reverse(result.begin(), result.end());
    return result;
}

TVector<TInvMiIndex::TSearchResult> TInvMiIndex::GetNearestVectors(TSearchThreadData* ctx,
                                                                   const float* query,
                                                                   size_t topSize,
                                                                   size_t minClustersToSearch) const {
    ctx->Init(K, NumVectors, Dimension);

    float* rotatedQuery = ctx->RotatedQuery.data();
    MatrixMatrixMultiply({ RotationMatrix, Dimension, Dimension, CblasNoTrans, 1.0 },
                         { query, Dimension, 1 },
                         { rotatedQuery, Dimension, 1 });

    CalcDistToCentroids(Dimension,
                        Centroids1, CentroidNorms1.data(), K,
                        rotatedQuery,
                        ctx->Dist1.data());
    rotatedQuery += Dimension / 2;
    CalcDistToCentroids(Dimension,
                        Centroids2, CentroidNorms2.data(), K,
                        rotatedQuery,
                        ctx->Dist2.data());

    //GetNearestClusterIds(ctx, Max(topSize, 22000UL), minClustersToSearch);
    GetNearestClusterIds(ctx, topSize, minClustersToSearch);
    if (ByteVectors) {
        TVector<i8> byteQuery(Dimension);
        for (size_t i = 0; i < Dimension; ++i) {
            byteQuery[i] = static_cast<i8>(query[i]);
        }
        return SelectNearestFromClusters(Dimension, AlignedDimension,
                                         ByteVectors, byteQuery.data(),
                                         ctx->FoundClusters,
                                         topSize);
    }
    return SelectNearestFromClusters(Dimension, Dimension,
                                     Vectors, query,
                                     ctx->FoundClusters,
                                     topSize);

    TLimitedHeap<TSearchResult, TLess<TSearchResult>> top(topSize, TLess<TSearchResult>());
    size_t numClusters = 0;
    size_t numVectors = 0;
    size_t numEmpty = 0;
    for (size_t clusterId : ctx->NearestClusterIds) {
        auto cluster = GetCluster(clusterId);
    //for (const auto& cluster : ctx->FoundClusters) {
        ++numClusters;
        numEmpty += cluster.empty();
        for (ui32 vectorId : cluster) {
            ++numVectors;
            float dist = DotProduct(query, Vectors + vectorId * Dimension, Dimension);
            top.Insert({ dist, vectorId });
        }
        if (numVectors >= 30000) break;
    }
    //Cerr << numClusters << ' ' << numVectors << ' ' << numEmpty << Endl;
    //Cerr << numClusters << ' ' << numVectors << Endl;
    TVector<TSearchResult> result;
    result.reserve(top.GetSize());
    for (; !top.IsEmpty(); top.PopMin()) {
        result.push_back(top.GetMin());
    }
    std::reverse(result.begin(), result.end());
    return result;
    /*
    TVector<TSearchResult> result;
    for (size_t clusterId : ctx->NearestClusterIds) {
        for (ui32 vectorId : GetCluster(clusterId)) {
            float dist = DotProduct(query, Vectors + vectorId * Dimension, Dimension);
            result.push_back({ dist, vectorId });
        }
    }
    size_t resultSize = Min(result.size(), topSize);
    std::partial_sort(result.begin(), result.begin() + resultSize, result.end());
    result.resize(resultSize);
    return result;
    */
}

