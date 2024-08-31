#pragma once
#include "build_routines.h"

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/operation.h>

#include <util/generic/vector.h>

namespace NYtHnsw {
/**
 * @param inputTable    table with columns {shard_id:ui64 doc_id:whatever item:str}
 *                      Index is build for each shard_id independently.
 *                      Column item:str stores serialized items.
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
                const TString& outputTable,
                const TMaybe<TString>& docIdMappingTable = Nothing()) {
    const TItemsTable itemsTable(client);
    const auto shardSizes = EnumerateShardItems(client, inputTable, itemsTable);

    THnswInternalYtBuildOptions internalOpts(opts);
    UpdateItemLimits(internalOpts, GetMaxShardSize(shardSizes));
    Y_ENSURE(CheckShardSizes(internalOpts, shardSizes));

    if (docIdMappingTable) {
        MakeDocIdMapping(client, itemsTable, *docIdMappingTable);
    }

    client->Create(outputTable, NYT::NT_TABLE, NYT::TCreateOptions().Recursive(true).IgnoreExisting(true));
    BuildUpperLevels<TDistanceTraits, TItemStorage>(internalOpts, distanceTraits, client, itemsTable, outputTable, shardSizes);
    BuildLowerLevels<TDistanceTraits, TItemStorage>(internalOpts, distanceTraits, client, itemsTable, outputTable, shardSizes);
    TrimNeighbors<TDistanceTraits, TItemStorage>(internalOpts, distanceTraits, client, itemsTable, outputTable, shardSizes);
    ConstructIndexData(internalOpts, client, outputTable, shardSizes);
}

}

/**
 * Use this macro to register BuildIndex jobs.
 */
#define REGISTER_HNSW_YT_BUILD_JOBS(TDistanceTraits, TItemStorage) \
    REGISTER_REDUCER(NYtHnsw::TBuildUpperLevelsReducer<TDistanceTraits, TItemStorage>); \
    REGISTER_REDUCER(NYtHnsw::TBuildPartitionReducer<TDistanceTraits, TItemStorage>); \
    REGISTER_REDUCER(NYtHnsw::TJoinPartitionsReducer<TDistanceTraits, TItemStorage>); \
    REGISTER_REDUCER(NYtHnsw::TTrimNeighborsReducer<TDistanceTraits, TItemStorage>);

