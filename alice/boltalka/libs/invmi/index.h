#pragma once
#include <util/generic/array_ref.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/memory/blob.h>


class TInvMiModel {
public:
    TInvMiModel(const TString& filename, size_t dimension);
    TInvMiModel(const TBlob& blob, size_t dimension);

    size_t GetClusterId(size_t centroidId1, size_t centroidId2) const;
    size_t GetCentroidId1(size_t clusterId) const;
    size_t GetCentroidId2(size_t clusterId) const;
    size_t GetNumClusters() const;

protected:
    size_t Dimension;

    TBlob ModelData;
    ui32 K;
    const float* RotationMatrix;
    const float* Centroids1;
    const float* Centroids2;
    const unsigned char* ModelDataEnd;

    TVector<float> CentroidNorms1;
    TVector<float> CentroidNorms2;

private:
    void LoadFromBlob(const TBlob& blob);
};

class TInvMiIndex : public TInvMiModel {
public:
    struct TSearchResult {
        float Dist;
        ui32 Id;
        bool operator<(const TSearchResult& other) const {
            return Dist > other.Dist || Dist == other.Dist && Id < other.Id;
        }
    };

    struct TSearchThreadData {
        TVector<float> RotatedQuery;
        TVector<float> Dist1;
        TVector<float> Dist2;
        TVector<std::pair<float, ui32>> DistToId1;
        TVector<std::pair<float, ui32>> DistToId2;
        TVector<ui32> NumVisitedColumns;
        TVector<ui32> NearestClusterIds;
        TVector<ui32> NearestVectorIds;
        TVector<TConstArrayRef<ui32>> FoundClusters;
        void Init(size_t K, size_t numVectors, size_t dimension);
    };

    class TSearcher {
    public:
        TSearcher(const TInvMiIndex* index);
        TVector<TSearchResult> GetNearestVectors(const float* query,
                                                 size_t topSize,
                                                 size_t minClustersToSearch = 0);
    private:
        const TInvMiIndex* Index;
        TSearchThreadData Data;
    };

public:
    TInvMiIndex(const TString& invMiModelFilename, size_t dimension);
    TInvMiIndex(const TBlob& invMiModelBlob, size_t dimension);

    void BuildIndex(const TBlob& vectorsBlob);
    void BuildIndex(const TString& vectorsFilename);

    void LinkToVectors(const TBlob& vectorsBlob);
    void LinkToVectors(const TString& vectorsFilename);

    void ConvertToByteVectors();

    void SerializeAsInvMiModel(const TString& filename) const;

    TSearcher CreateSearcher() const;
    TConstArrayRef<ui32> GetCluster(size_t clusterId) const;
    TVector<TSearchResult> GetNearestVectors(TSearchThreadData* ctx,
                                             const float* query,
                                             size_t topSize,
                                             size_t minClustersToSearch = 0) const;

private:
    void TryLoadClustersFromInvMiModel();

    float CalcAverageClusterSize() const;
    void InitNonEmptyCluster();

    void GetNearestClusterIds(TSearchThreadData* ctx,
                              size_t minVectorsToSearch,
                              size_t minClustersToSearch) const;

private:
    TBlob VectorData;
    size_t NumVectors = 0;
    const float* Vectors = nullptr;

    size_t AlignedDimension = 0;
    const i8* ByteVectors = nullptr;

    const ui32* VectorIds = nullptr; // TODO(alipov): serialize as ui32 too
    const ui32* CumClusterSizes = nullptr; // TODO(alipov): change to offsets
    struct TClusterInfo {
        ui32 X, Y;
        TConstArrayRef<ui32> Vectors;
        bool operator<(const TClusterInfo&) const { return false; }
    };
    TVector<TClusterInfo> NonEmpty;
    TVector<ui32> VectorIdsData;
    TVector<ui32> CumClusterSizesData;
};

