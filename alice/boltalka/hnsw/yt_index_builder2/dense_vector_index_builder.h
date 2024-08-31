#pragma once
#include "index_builder.h"

#include <library/cpp/hnsw/index_builder/dense_vector_index_builder.h>

#include <util/generic/ptr.h>
#include <util/memory/blob.h>
#include <util/stream/buffer.h>
#include <util/ysaveload.h>

namespace NYtHnsw {

template<class TVectorComponent,
         class TDistance,
         class TDistanceResult = typename TDistance::TResult,
         class TDistanceLess = typename TDistance::TLess>
using TDenseDistanceTraits = NHnsw::TDistanceTraits<NHnsw::TDistanceWithDimension<TVectorComponent, TDistance>, TDistanceResult, TDistanceLess>;

template<class TVectorComponent>
class TYtDenseVectorStorage {
public:
    using TItem = const TVectorComponent*;

    explicit TYtDenseVectorStorage(size_t totalNumItemsBytes = 0)
        : Output(totalNumItemsBytes)
    {
    }

    static const TVectorComponent* ParseItemFromString(TString &itemStr) {
        return reinterpret_cast<const TVectorComponent*>(itemStr.data());
    }

    void AddItemFromString(const TString& itemStr) {
        Output.Write(itemStr.data(), itemStr.size());
        Y_VERIFY(itemStr.size() % sizeof(TVectorComponent) == 0);
        size_t dimension = itemStr.size() / sizeof(TVectorComponent);
        Y_VERIFY(Dimension == 0 || Dimension == dimension);
        Dimension = dimension;
    }

    void Finalize() {
        Y_VERIFY(Dimension != 0);
        Storage = MakeHolder<NHnsw::TDenseVectorStorage<TVectorComponent>>(TBlob::FromBuffer(Output.Buffer()), Dimension);
    }

    ui64 GetNumItems() const {
        return Storage->GetNumItems();
    }

    const TVectorComponent* GetItem(size_t id) const {
        return Storage->GetItem(id);
    }

    void Save(IOutputStream* out) const {
        ::Save(out, Dimension);
        ::Save(out, GetNumItems());
        ::SavePodArray(out, GetItem(0), Dimension * GetNumItems());
    }

    void Load(IInputStream* in) {
        ::Load(in, Dimension);
        ui64 numItems;
        ::Load(in, numItems);
        const ui64 numBytes = Dimension * numItems * sizeof(TVectorComponent);
        Output.Buffer().Resize(numBytes);
        ::LoadPodArray(in, Output.Buffer().Begin(), numBytes);
        Storage = MakeHolder<NHnsw::TDenseVectorStorage<TVectorComponent>>(TBlob::FromBuffer(Output.Buffer()), Dimension);
    }

private:
    ui64 Dimension = 0;
    THolder<NHnsw::TDenseVectorStorage<TVectorComponent>> Storage;
    TBufferOutput Output;
};

template<class TVectorComponent,
         class TDistance,
         class TDistanceResult = typename TDistance::TResult,
         class TDistanceLess = typename TDistance::TLess>
void BuildDenseVectorIndex(const THnswYtBuildOptions& opts,
                           size_t dimension,
                           NYT::IClientPtr client,
                           const TString& inputTable,
                           const TString& outputTable,
                           const TMaybe<TString>& docIdMappingTable = Nothing(),
                           const TDistance& distance = {},
                           const TDistanceLess& distanceLess = {}) {
    NHnsw::TDistanceWithDimension<TVectorComponent, TDistance> distanceWithDimension(distance, dimension);
    using TDistanceTraits = TDenseDistanceTraits<TVectorComponent, TDistance, TDistanceResult, TDistanceLess>;
    TDistanceTraits distanceTraits(distanceWithDimension, distanceLess);
    using TItemStorage = TYtDenseVectorStorage<TVectorComponent>;
    BuildIndex<TDistanceTraits, TItemStorage>(opts, distanceTraits, client, inputTable, outputTable, docIdMappingTable);
}

}

#define REGISTER_HNSW_YT_DENSE_VECTOR_BUILD_JOBS(TDistanceTraits, TItemStorage) \
    REGISTER_HNSW_YT_BUILD_JOBS(TDistanceTraits, TItemStorage); \
    Y_DECLARE_PODTYPE(TDistanceTraits);

