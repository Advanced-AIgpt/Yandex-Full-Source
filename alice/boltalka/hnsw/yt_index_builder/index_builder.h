#pragma once
#include "build_routines.h"

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/operation.h>
#include <mapreduce/yt/util/temp_table.h>

#include <util/generic/vector.h>

namespace NHnsw {
/**
 * @param inputTable    table with columns {shard_id:ui64 doc_id:whatever item:str}
 *                      Index is build for each shar_id independently.
 *                      Column item:str stores searialized items.
 * @param outputTable   Feed this table to index_writer.h:WriteIndex to download built index.
 *
 * See dense_vector_index_builder.h for usage example.
 */
template<class TDistanceTraits,
         class TItemStorage>
void BuildIndex(const THnswYtBuildOptions& opts,
                const TDistanceTraits& distanceTraits,
                NYT::IClientPtr client,
                const TString& inputTable,
                const TString& outputTable) {
    const THnswInternalYtBuildOptions internalOpts(opts);

    NYT::TTempTable itemsTable(client);
    size_t numItems = NPrivate::EnumerateItems(client, inputTable, itemsTable.Name());
    Y_VERIFY(numItems > internalOpts.MaxNeighbors);

    auto levelSizes = GetLevelSizes(numItems, internalOpts.LevelSizeDecay);

    client->Remove(outputTable, NYT::TRemoveOptions().Force(true));
    client->Create(outputTable, NYT::NT_TABLE, NYT::TCreateOptions().Recursive(true));

    size_t prevLevelSize = 0;
    for (size_t level = levelSizes.size(); level-- > 0; ) {
        Cerr << "Building level " << level << " size " << levelSizes[level] << Endl;
        size_t batchSize = level == 0 ? internalOpts.BatchSize : internalOpts.UpperLevelBatchSize;
        size_t levelSize = levelSizes[level];
        BuildLevel<TDistanceTraits, TItemStorage>(internalOpts, distanceTraits, prevLevelSize, level, levelSize, batchSize, client, itemsTable.Name(), outputTable);
        prevLevelSize = levelSize;
    }
    ConstructIndexData<typename TDistanceTraits::TNeighbors>(internalOpts, numItems, client, outputTable);
    Cerr << Endl << "Done" << Endl;
}

}

/**
 * Use this macro to register BuildIndex jobs.
 */
#define REGISTER_HNSW_YT_BUILD_JOBS(TDistanceTraits, TItemStorage) \
    REGISTER_REDUCER(NHnsw::TFindApproximateNeighborsReducer<TDistanceTraits, TItemStorage>); \
    REGISTER_REDUCER(NHnsw::TAddExactNeighborsReducer<TDistanceTraits, TItemStorage>); \
    REGISTER_MAPPER(NHnsw::TAppendBatchToLevelsMapper<TDistanceTraits::TNeighbors>); \
    REGISTER_REDUCER(NHnsw::TJoinNeighborsReducer<TDistanceTraits::TNeighbors>); \
    REGISTER_REDUCER(NHnsw::TTrimNeighborsReducer<TDistanceTraits, TItemStorage>); \
    REGISTER_MAPPER(NHnsw::TConstructIndexDataMapper<TDistanceTraits::TNeighbors>);

