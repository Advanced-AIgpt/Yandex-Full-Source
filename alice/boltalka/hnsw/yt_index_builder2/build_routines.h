#pragma once
#include "internal_build_options.h"
#include "chunked_stream.h"

#include <library/cpp/hnsw/index/index_base.h>
#include <library/cpp/hnsw/index_builder/index_builder.h>
#include <library/cpp/hnsw/index_builder/index_writer.h>

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/operation.h>
#include <mapreduce/yt/util/temp_table.h>

#include <util/generic/maybe.h>
#include <util/stream/buffer.h>
#include <util/ysaveload.h>
#include <util/system/hp_timer.h>

#include <alice/boltalka/hnsw/yt_index_builder2/types.pb.h>

namespace NYtHnsw {

static constexpr size_t INDEX_CHUNK_SIZE = 1 << 16;

class TItemsTable {
public:
    explicit TItemsTable(NYT::IClientBasePtr client)
        : Client(client)
        , Storage(client)
        , ItemSize()
    {
    }

    NYT::TRichYPath GetPath() const {
        return Storage.Name();
    }

    size_t EstimateItemSize() const {
        if (!ItemSize) {
            ItemSize = AverageItemSize();
        }
        return ItemSize;
    }

private:
    size_t AverageItemSize() const {
        static const constexpr size_t ITEMS_TO_TAKE = 100;
        size_t total = 0;
        size_t count = 0;
        for (auto reader = Client->CreateTableReader<TEmbeddingPB>(GetPath()); reader->IsValid() && count < ITEMS_TO_TAKE; reader->Next()) {
            total += reader->GetRow().GetItem().size();
            ++count;
        }
        Y_VERIFY(count != 0, "Table is empty");
        return total / count;
    }

private:
    const NYT::IClientBasePtr Client;
    const NYT::TTempTable Storage;
    mutable size_t ItemSize;
};

size_t GetMaxShardSize(const THashMap<ui64, ui64>& shardSizes);
bool CheckShardSizes(const THnswInternalYtBuildOptions& opts, const THashMap<ui64, ui64>& shardSizes);

//! @return shard sizes
THashMap<ui64, ui64> EnumerateShardItems(NYT::IClientPtr client,
                                         const TString& inputTable,
                                         const TItemsTable& itemsTable);

void MakeDocIdMapping(NYT::IClientPtr client,
                      const TItemsTable& itemsTable,
                      const TString& mappingTable);

template<class TItemStorage>
void ReadItems(NYT::TTableReader<NYT::TNode>* input,
               size_t tableIndex,
               TItemStorage* itemStorage,
               size_t maxItems = Max<size_t>()) {

    for (; input->IsValid() && input->GetTableIndex() == tableIndex; input->Next()) {
        const auto& row = input->GetRow();
        itemStorage->AddItemFromString(row["item"].AsString());
        if (--maxItems == 0) {
            break;
        }
    }
    itemStorage->Finalize();
}

template<class TTo, class TFromElem>
TTo ContainerReinterpretCast(const TFromElem* begin, const TFromElem* end) {
    using TToElem = typename TTo::value_type;
    const auto* toBegin = reinterpret_cast<const TToElem*>(begin);
    const auto* toEnd = reinterpret_cast<const TToElem*>(end);
    return { toBegin, toEnd };
}

template<class TTo, class TFrom>
TTo ContainerReinterpretCast(const TFrom& from) {
    return ContainerReinterpretCast<TTo>(from.data(), from.data() + from.size());
}

template<class TDistanceTraits,
         class TItemStorage>
class TBuildUpperLevelsReducer : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    TBuildUpperLevelsReducer() = default;

    TBuildUpperLevelsReducer(const THnswInternalYtBuildOptions& opts,
                             const TDistanceTraits& distanceTraits,
                             const THashMap<ui64, ui64>& shardSizes,
                             size_t estimatedItemSize)
        : Opts(opts)
        , DistanceTraits(distanceTraits)
        , ShardSizes(shardSizes)
        , EstimatedItemSize(estimatedItemSize)
    {
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        const ui64 shardId = input->GetRow()["shard_id"].AsUint64();

        TVector<size_t> levelSizes = NHnsw::GetLevelSizes(ShardSizes[shardId], Opts.LevelSizeDecay);
        const size_t firstLevelToBuild = std::lower_bound(levelSizes.begin(), levelSizes.end(), Opts.MaxItemsForLocalBuild, TGreater<size_t>()) - levelSizes.begin();
        Y_VERIFY(firstLevelToBuild < levelSizes.size());
        const size_t numItems = levelSizes[firstLevelToBuild];

        THPTimer watch;
        TItemStorage itemStorage(numItems * EstimatedItemSize);
        ReadItems(input, /*tableIndex*/0, &itemStorage, numItems);
        Cerr << "read items: " << watch.PassedReset() << " seconds" << Endl;

        auto indexData = BuildIndexWithTraits(Opts, DistanceTraits, itemStorage);
        Cerr << "build index: " << watch.PassedReset() << " seconds" << Endl;

        const ui32* neighborsData = indexData.FlatLevels.data();
        for (size_t levelId = firstLevelToBuild; levelId < levelSizes.size(); ++levelId) {
            const size_t levelSize = levelSizes[levelId];
            const size_t numNeighbors = Min(levelSize - 1, Opts.MaxNeighbors);
            for (size_t id = 0; id < levelSize; ++id) {
                const auto neighborsStr = ContainerReinterpretCast<TString>(
                    neighborsData, neighborsData + numNeighbors);
                output->AddRow(NYT::TNode()
                    ("shard_id", shardId)
                    ("level_id", levelId)
                    ("id", id)
                    ("neighbors", neighborsStr)
                );
                neighborsData += numNeighbors;
            }
        }
        Cerr << "write index: " << watch.PassedReset() << " seconds" << Endl;
    }

    static ui64 EstimateMemoryLimit(const THnswInternalYtBuildOptions& opts, size_t itemSize) {
        const ui64 storageSize = itemSize * opts.MaxItemsForLocalBuild;
        const ui64 indexSize = opts.MaxNeighbors * sizeof(typename TDistanceTraits::TNeighbor) * opts.MaxItemsForLocalBuild;
        const ui64 additionalMemorySize = (opts.SearchNeighborhoodSize + opts.NumExactCandidates)
            * sizeof(typename TDistanceTraits::TNeighbor) * opts.BatchSize * 3 * opts.NumThreads;
        return storageSize + indexSize + additionalMemorySize;
    }

    Y_SAVELOAD_JOB(Opts, DistanceTraits, ShardSizes, EstimatedItemSize);

private:
    THnswInternalYtBuildOptions Opts;
    TDistanceTraits DistanceTraits;
    THashMap<ui64, ui64> ShardSizes;
    size_t EstimatedItemSize;
};

/**
 * @brief Builds upper levels of HNSW index that are not greater than opts.MaxLevelSizeForLocalBuild
 *
 * @param itemsTable    {shard_id:ui64 id:ui64 item:str}
 * @param outputTable   {shard_id:ui64 level_id:ui64 id:ui64 neighbors:str}
 */
template<class TDistanceTraits,
         class TItemStorage>
void BuildUpperLevels(const THnswInternalYtBuildOptions& opts,
                      const TDistanceTraits& distanceTraits,
                      NYT::IClientPtr client,
                      const TItemsTable& itemsTable,
                      const TString& outputTable,
                      const THashMap<ui64, ui64>& shardSizes) {
    NYT::TUserJobSpec reducerSpec;
    const auto itemSize = itemsTable.EstimateItemSize();
    const ui64 memoryLimit = opts.AdditionalMemoryLimit +
        TBuildUpperLevelsReducer<TDistanceTraits, TItemStorage>::EstimateMemoryLimit(opts, itemSize);
    reducerSpec.MemoryLimit(memoryLimit);

    NYT::TNode jobSpec = NYT::TNode()
        ("reducer", NYT::TNode()
            ("cpu_limit", opts.CpuLimit));

    client->Reduce(
        NYT::TReduceOperationSpec()
            .AddInput<NYT::TNode>(itemsTable.GetPath())
            .AddOutput<NYT::TNode>(NYT::TRichYPath(outputTable).SortedBy({"shard_id", "level_id", "id"}))
            .ReduceBy({ "shard_id" })
            .ReducerSpec(reducerSpec),
        new TBuildUpperLevelsReducer<TDistanceTraits, TItemStorage>(opts, distanceTraits, shardSizes, itemSize),
        NYT::TOperationOptions().Spec(jobSpec)
    );
}


void EnumeratePartitions(const THnswInternalYtBuildOptions& opts,
                         NYT::IClientPtr client,
                         const TItemsTable& itemsTable,
                         const TItemsTable& partitionedItemsTable,
                         const THashMap<ui64, ui64>& shardSizes,
                         size_t numPartitions,
                         size_t levelId);

template<class TDistanceTraits,
         class TItemStorage>
class TBuildPartitionReducer : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    TBuildPartitionReducer() = default;

    TBuildPartitionReducer(const THnswInternalYtBuildOptions& opts,
                           const TDistanceTraits& distanceTraits)
        : Opts(opts)
        , DistanceTraits(distanceTraits)
    {
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        const ui64 shardId = input->GetRow()["shard_id"].AsUint64();
        const ui64 partId = input->GetRow()["part_id"].AsUint64();
        const ui64 partSize = input->GetRow()["part_size"].AsUint64();

        THPTimer watch;
        const ui64 estimatedStorageSize = partSize * input->GetRow()["item"].AsString().size();
        TItemStorage itemStorage(estimatedStorageSize);
        ReadItems(input, /*tableIndex*/0, &itemStorage);
        Cerr << "read items: " << watch.PassedReset() << " seconds" << Endl;

        TBufferOutput indexBuffer;
        {
            auto indexData = BuildIndexWithTraits(Opts, DistanceTraits, itemStorage);
            NHnsw::WriteIndex(indexData, indexBuffer);
        }
        Cerr << "build index: " << watch.PassedReset() << " seconds" << Endl;

        ui64 indexChunkId = 0;
        TChunkedOutputStream out(INDEX_CHUNK_SIZE, [&](const TBuffer& chunk) {
            output->AddRow(NYT::TNode()
                ("shard_id", shardId)
                ("part_id", partId)
                ("part_size", partSize)
                ("index_chunk_id", indexChunkId++)
                ("index_chunk", TString(chunk.Begin(), chunk.End()))
            );
        });
        ::Save(&out, indexBuffer.Buffer());
        ::Save(&out, itemStorage);
        out.Finish();
        Cerr << "write index: " << watch.PassedReset() << " seconds" << Endl;
    }

    static ui64 EstimateMemoryLimit(const THnswInternalYtBuildOptions& opts, size_t itemSize) {
        return TBuildUpperLevelsReducer<TDistanceTraits, TItemStorage>::EstimateMemoryLimit(opts, itemSize);
    }

    Y_SAVELOAD_JOB(Opts, DistanceTraits);

private:
    THnswInternalYtBuildOptions Opts;
    TDistanceTraits DistanceTraits;
};

/**
 * @brief Builds opts.NumPartions vanilla HNSW indexes in each shard.
 * Produces a table with chunked HNSW index data and items for that partitions.
 *
 * @param itemsTable    {shard_id:ui64 part_id:ui64 id:ui64 item:str part_size:ui64}
 * @param outputTable   {shard_id:ui64 part_id:ui64 part_size:ui64 index_chunk_id:ui64 index_chunk:str}
 */
template<class TDistanceTraits,
         class TItemStorage>
void BuildPartitions(const THnswInternalYtBuildOptions& opts,
                     const TDistanceTraits& distanceTraits,
                     NYT::IClientPtr client,
                     const TItemsTable& partitionedItemsTable,
                     const TString& outputTable) {
    NYT::TUserJobSpec reducerSpec;
    const ui64 memoryLimit = opts.AdditionalMemoryLimit +
        TBuildPartitionReducer<TDistanceTraits, TItemStorage>::EstimateMemoryLimit(opts, partitionedItemsTable.EstimateItemSize());
    reducerSpec.MemoryLimit(memoryLimit);

    NYT::TNode jobSpec = NYT::TNode()
        ("reducer", NYT::TNode()
            ("cpu_limit", opts.CpuLimit));

    client->Reduce(
        NYT::TReduceOperationSpec()
            .AddInput<NYT::TNode>(partitionedItemsTable.GetPath())
            .AddOutput<NYT::TNode>(NYT::TRichYPath(outputTable).SortedBy({ "shard_id", "part_id", "index_chunk_id" }))
            .ReduceBy({ "shard_id", "part_id" })
            .ReducerSpec(reducerSpec),
        new TBuildPartitionReducer<TDistanceTraits, TItemStorage>(opts, distanceTraits),
        NYT::TOperationOptions().Spec(jobSpec)
    );
}

template<class TDistanceTraits,
         class TItemStorage>
class TJoinPartitionsReducer : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
    using TItem = typename TItemStorage::TItem;
    using TNeighbor = typename TDistanceTraits::TNeighbor;
    using TNeighbors = typename TDistanceTraits::TNeighbors;
    using TNeighborMaxQueue = typename TDistanceTraits::TNeighborMaxQueue;

public:
    TJoinPartitionsReducer() = default;

    TJoinPartitionsReducer(const THnswInternalYtBuildOptions& opts,
                           const TDistanceTraits& distanceTraits,
                           size_t levelId,
                           size_t estimatedStorageSize)
        : Opts(opts)
        , DistanceTraits(distanceTraits)
        , LevelId(levelId)
        , EstimatedStorageSize(estimatedStorageSize)
    {
    }

    void Start(NYT::TTableWriter<NYT::TNode>*) override {
        NPar::LocalExecutor().RunAdditionalThreads(Opts.NumThreads - 1);
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        const ui64 shardId = input->GetRow()["shard_id"].AsUint64();
        const ui64 firstItemId = input->GetRow()["id"].AsUint64();

        THPTimer watch;
        TItemStorage itemStorage(EstimatedStorageSize);
        ReadItems(input, /*tableIndex*/0, &itemStorage);
        Cerr << "read items: " << watch.PassedReset() << " seconds" << Endl;

        TVector<TNeighborMaxQueue> nearestNeighborsForItems(itemStorage.GetNumItems(),
                                                            TNeighborMaxQueue(DistanceTraits.DistanceLess));
        for (auto& topItems : nearestNeighborsForItems) {
            topItems.Container().reserve(Opts.NumNearestCandidates + 1);
        }

        TChunkedInputStream in([&](TBuffer& chunk) {
            Y_VERIFY(input->IsValid());
            const auto& chunkStr = input->GetRow()["index_chunk"].AsString();
            chunk.Assign(chunkStr.data(), chunkStr.size());
            input->Next();
        });

        while (input->IsValid()) {
            const ui64 partId = input->GetRow()["part_id"].AsUint64();
            const ui64 partSize = input->GetRow()["part_size"].AsUint64();
            Cerr << "joining part#" << partId << Endl;

            TBuffer indexBuffer;
            ::Load(&in, indexBuffer);
            NHnsw::THnswIndexBase partIndex(TBlob::FromBuffer(indexBuffer));
            Cerr << "read index: " << watch.PassedReset() << " seconds" << Endl;

            TItemStorage partItemStorage;
            ::Load(&in, partItemStorage);
            Cerr << "read items: " << watch.PassedReset() << " seconds" << Endl;

            auto task = [&](int id) {
                const auto& curItem = itemStorage.GetItem(id);
                const ui64 curItemIdInShard = firstItemId + id;
                auto nearest = partIndex.GetNearestNeighbors(curItem,
                                                             Opts.NumNearestCandidates,
                                                             Opts.SearchNeighborhoodSize,
                                                             partItemStorage,
                                                             DistanceTraits.Distance,
                                                             DistanceTraits.DistanceLess);
                auto& topItems = nearestNeighborsForItems[id];
                for (const auto& neighbor : nearest) {
                    const auto neighborDist = neighbor.Dist;
                    const auto neighborIdInShard = neighbor.Id + partId * partSize;
                    if (neighborIdInShard == curItemIdInShard) {
                        continue;
                    }
                    if (topItems.size() < Opts.NumNearestCandidates || DistanceTraits.DistanceLess(neighborDist, topItems.top().Dist)) {
                        topItems.push({neighborDist, neighborIdInShard});
                        if (topItems.size() > Opts.NumNearestCandidates) {
                            topItems.pop();
                        }
                    }
                }
            };
            NPar::LocalExecutor().ExecRange(task, 0, itemStorage.GetNumItems(), NPar::TLocalExecutor::WAIT_COMPLETE);
            Cerr << "find neighbors: " << watch.PassedReset() << " seconds" << Endl;
        }

        THashMap<ui32, TVector<ui32>> reverseNeighbors;
        for (size_t i = 0; i < itemStorage.GetNumItems(); ++i) {
            const ui64 curItemIdInShard = firstItemId + i;

            auto& topItems = nearestNeighborsForItems[i];
            TVector<ui32> neighbors;
            neighbors.reserve(Opts.NumNearestCandidates);
            for (; !topItems.empty(); topItems.pop()) {
                neighbors.push_back(topItems.top().Id);
            }
            std::reverse(neighbors.begin(), neighbors.end());
            output->AddRow(NYT::TNode()
                ("shard_id", shardId)
                ("level_id", LevelId)
                ("id", curItemIdInShard)
                ("neighbors", ContainerReinterpretCast<TString>(neighbors))
            );

            for (ui32 neighborId : neighbors) {
                reverseNeighbors[neighborId].push_back(curItemIdInShard);
            }
        }
        Cerr << "write neighbors: " << watch.PassedReset() << " seconds" << Endl;
        for (const auto& pair : reverseNeighbors) {
            ui32 id = pair.first;
            const auto& neighbors = pair.second;
            output->AddRow(NYT::TNode()
                ("shard_id", shardId)
                ("level_id", LevelId)
                ("id", id)
                ("neighbors", ContainerReinterpretCast<TString>(neighbors))
            );
        }
        Cerr << "write reverse neighbors: " << watch.PassedReset() << " seconds" << Endl;
    }

    static ui64 EstimateMemoryLimit(const THnswInternalYtBuildOptions& opts, size_t itemSize, size_t estimatedNumRowsPerJob) {
        const ui64 storageSize = itemSize * estimatedNumRowsPerJob;
        const ui64 partitionStorageSize = itemSize * opts.MaxItemsForLocalBuild;
        const ui64 partitionIndexSize = opts.MaxItemsForLocalBuild * opts.MaxNeighbors * sizeof(ui32) * 2;
        const ui64 nearestCandidates = opts.NumNearestCandidates
            * sizeof(typename TDistanceTraits::TNeighbor) * estimatedNumRowsPerJob * 2 * opts.NumThreads;
        return storageSize + partitionStorageSize + partitionIndexSize + nearestCandidates;
    }

    Y_SAVELOAD_JOB(Opts, DistanceTraits, LevelId, EstimatedStorageSize);

private:
    THnswInternalYtBuildOptions Opts;
    TDistanceTraits DistanceTraits;
    size_t LevelId;
    size_t EstimatedStorageSize;
};

class TJoinNeighborsReducer : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        const ui64 shardId = input->GetRow()["shard_id"].AsUint64();
        const ui64 levelId = input->GetRow()["level_id"].AsUint64();
        const ui64 id = input->GetRow()["id"].AsUint64();
        TVector<ui32> neighbors;
        for (; input->IsValid(); input->Next()) {
            const auto& neighborsStr = input->GetRow()["neighbors"].AsString();
            const auto curNeighbors = ContainerReinterpretCast<TVector<ui32>>(neighborsStr);
            neighbors.insert(neighbors.end(), curNeighbors.begin(), curNeighbors.end());
        }
        std::sort(neighbors.begin(), neighbors.end());
        neighbors.erase(std::unique(neighbors.begin(), neighbors.end()), neighbors.end());

        output->AddRow(NYT::TNode()
            ("shard_id", shardId)
            ("level_id", levelId)
            ("id", id)
            ("neighbors", ContainerReinterpretCast<TString>(neighbors))
        );
    }
};

/**
 * @brief Finds nearest opts.NumNearestCandidates items for each item in shard,
 * using HNSW indexes from partitions.
 * Appends resulting neighbors to outputTable with corresponding level_id.
 *
 * @param itemsTable    {shard_id:ui64 part_id:ui64 id:ui64 item:str part_size:ui64}
 * @param outputTable   {shard_id:ui64 level_id:ui64 id:ui64 neighbors:str}
 */
template<class TDistanceTraits,
         class TItemStorage>
void JoinPartitions(const THnswInternalYtBuildOptions& opts,
                    const TDistanceTraits& distanceTraits,
                    NYT::IClientPtr client,
                    const TItemsTable& partitionedItems,
                    const TString& partitionsTable,
                    const TString& outputTable,
                    size_t levelId,
                    size_t maxLevelSize,
                    size_t numShards) {
    const size_t itemSize = partitionedItems.EstimateItemSize();
    const size_t estimatedNumRowsPerJob = Max<size_t>(maxLevelSize / numShards, 1);
    NYT::TUserJobSpec reducerSpec;
    const ui64 memoryLimit = opts.AdditionalMemoryLimit +
        TJoinPartitionsReducer<TDistanceTraits, TItemStorage>::EstimateMemoryLimit(opts, itemSize, estimatedNumRowsPerJob);
    reducerSpec.MemoryLimit(memoryLimit);

    NYT::TNode jobSpec = NYT::TNode()
        ("reducer", NYT::TNode()
            ("cpu_limit", opts.CpuLimit));

    if (opts.NumJoinPartitionsJobs != NHnsw::THnswBuildOptions::AutoSelect) {
        jobSpec("job_count", opts.NumJoinPartitionsJobs * numShards)
                ("enable_job_splitting", false);
    }

    client->JoinReduce(
        NYT::TJoinReduceOperationSpec()
            .AddInput<NYT::TNode>(partitionedItems.GetPath())
            .AddInput<NYT::TNode>(NYT::TRichYPath(partitionsTable).Foreign(true))
            .AddOutput<NYT::TNode>(NYT::TRichYPath(outputTable).Append(true))
            .JoinBy({ "shard_id" })
            .ReducerSpec(reducerSpec),
        new TJoinPartitionsReducer<TDistanceTraits, TItemStorage>(opts, distanceTraits, levelId, estimatedNumRowsPerJob * itemSize),
        NYT::TOperationOptions().Spec(jobSpec)
    );
}

/**
 * @brief Builds lower levels of HNSW index.
 * Appends levels to the outputTable.
 *
 * @param itemsTable    {shard_id:ui64 id:ui64 item:str}
 * @param outputTable   {shard_id:ui64 level_id:ui64 id:ui64 neighbors:str}
 */
template<class TDistanceTraits,
         class TItemStorage>
void BuildLowerLevels(const THnswInternalYtBuildOptions& opts,
                      const TDistanceTraits& distanceTraits,
                      NYT::IClientPtr client,
                      const TItemsTable& itemsTable,
                      const TString& outputTable,
                      const THashMap<ui64, ui64>& shardSizes) {
    const TVector<size_t> maxLevelSizes = NHnsw::GetLevelSizes(GetMaxShardSize(shardSizes), opts.LevelSizeDecay);

    bool changed = false;
    for (size_t levelId = maxLevelSizes.size(); levelId-- > 0; ) {
        if (maxLevelSizes[levelId] <= opts.MaxItemsForLocalBuild) {
            Cerr << "Level#" << levelId << " was built locally" << Endl;
            continue;
        }
        Cerr << "Building level#" << levelId << ", max level size: " << maxLevelSizes[levelId] << Endl;
        const TItemsTable partitionedItems(client);

        const size_t numPartitions = (maxLevelSizes[levelId] + opts.MaxItemsForLocalBuild - 1) / opts.MaxItemsForLocalBuild;
        EnumeratePartitions(opts, client, itemsTable, partitionedItems, shardSizes, numPartitions, levelId);
        NYT::TTempTable partIndexes(client);
        BuildPartitions<TDistanceTraits, TItemStorage>(
            opts, distanceTraits, client, partitionedItems, partIndexes.Name());

        JoinPartitions<TDistanceTraits, TItemStorage>(
            opts, distanceTraits, client, partitionedItems, partIndexes.Name(), outputTable, levelId, maxLevelSizes[levelId], shardSizes.size());
        changed = true;
    }
    if (changed) {
        client->Sort(
            NYT::TSortOperationSpec()
                .AddInput(outputTable)
                .Output(outputTable)
                .SortBy({ "shard_id", "level_id", "id" })
        );
        client->Reduce(
            NYT::TReduceOperationSpec()
                .AddInput<NYT::TNode>(outputTable)
                .AddOutput<NYT::TNode>(NYT::TRichYPath(outputTable).SortedBy({ "shard_id", "level_id", "id" }))
                .ReduceBy({ "shard_id", "level_id", "id" }),
            new TJoinNeighborsReducer
        );
    }
}


template<class TDistanceTraits,
         class TItemStorage>
class TTrimNeighborsReducer : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
    using TNeighbor = typename TDistanceTraits::TNeighbor;
    using TNeighbors = typename TDistanceTraits::TNeighbors;

public:
    TTrimNeighborsReducer() = default;

    TTrimNeighborsReducer(const THnswInternalYtBuildOptions& opts,
                          const TDistanceTraits& distanceTraits,
                          const THashMap<ui64, ui64>& shardSizes,
                          size_t estimatedItemSize)
        : Opts(opts)
        , DistanceTraits(distanceTraits)
        , ShardSizes(shardSizes)
        , EstimatedItemSize(estimatedItemSize)
    {
    }

    void Start(NYT::TTableWriter<NYT::TNode>*) override {
        NPar::LocalExecutor().RunAdditionalThreads(Opts.NumThreads - 1);
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        const ui64 shardId = input->GetRow()["shard_id"].AsUint64();
        const ui64 shardSize = ShardSizes[shardId];

        THPTimer watch;
        TItemStorage itemStorage(shardSize * EstimatedItemSize);
        ReadItems(input, /*tableIndex*/0, &itemStorage);
        Cerr << "read items: " << watch.PassedReset() << " seconds" << Endl;

        TVector<ui64> itemIds;
        itemIds.reserve(Opts.BatchSize);
        TVector<TVector<ui32>> neighborsToTrim;
        neighborsToTrim.reserve(Opts.BatchSize);
        while (input->IsValid()) {
            const ui64 shardId = input->GetRow()["shard_id"].AsUint64();
            const ui64 levelId = input->GetRow()["level_id"].AsUint64();
            for (; input->IsValid() && input->GetRow()["level_id"].AsUint64() == levelId; input->Next()) {
                const auto& neighborsStr = input->GetRow()["neighbors"].AsString();
                neighborsToTrim.push_back(ContainerReinterpretCast<TVector<ui32>>(neighborsStr));
                itemIds.push_back(input->GetRow()["id"].AsUint64());
                if (neighborsToTrim.size() == Opts.BatchSize) {
                    TrimLevel(shardId, levelId, itemStorage, &neighborsToTrim, itemIds, output);
                    neighborsToTrim.clear();
                    itemIds.clear();
                }
            }
            if (!neighborsToTrim.empty()) {
                TrimLevel(shardId, levelId, itemStorage, &neighborsToTrim, itemIds, output);
                neighborsToTrim.clear();
                itemIds.clear();
            }
            Cerr << "trimmed level#" << levelId << ": " << watch.PassedReset() << " seconds" << Endl;
        }
    }

    void TrimLevel(ui64 shardId,
                   ui64 levelId,
                   const TItemStorage& itemStorage,
                   TVector<TVector<ui32>>* neighborsToTrim,
                   const TVector<ui64>& itemIds,
                   NYT::TTableWriter<NYT::TNode>* output) {
        Y_VERIFY(neighborsToTrim->size() == itemIds.size());

        auto task = [&](int id) {
            const auto& item = itemStorage.GetItem(itemIds[id]);
            auto& neighbors = (*neighborsToTrim)[id];
            if (neighbors.size() <= Opts.MaxNeighbors) {
                return;
            }
            TNeighbors neighborsWithDist;
            neighborsWithDist.reserve(neighbors.size());
            for (ui32 neighborId : neighbors) {
                neighborsWithDist.push_back({
                    DistanceTraits.Distance(item, itemStorage.GetItem(neighborId)),
                    neighborId
                });
            }
            NHnsw::TrimNeighbors(Opts, DistanceTraits, itemStorage, &neighborsWithDist);
            neighbors.clear();
            for (const auto& neighbor : neighborsWithDist) {
                neighbors.push_back(neighbor.Id);
            }
            neighbors.shrink_to_fit();
        };
        NPar::LocalExecutor().ExecRange(task, 0, neighborsToTrim->size(), NPar::TLocalExecutor::WAIT_COMPLETE);

        for (size_t i = 0; i < neighborsToTrim->size(); ++i) {
            output->AddRow(NYT::TNode()
                ("shard_id", shardId)
                ("level_id", levelId)
                ("id", itemIds[i])
                ("neighbors", ContainerReinterpretCast<TString>((*neighborsToTrim)[i]))
            );
        }
    }

    static ui64 EstimateMemoryLimit(const THnswInternalYtBuildOptions& opts, size_t itemSize, size_t maxShardSize) {
        const ui64 storageSize = itemSize * maxShardSize;
        const ui64 neighborsSize = opts.NumNearestCandidates
            * (sizeof(typename TDistanceTraits::TNeighbor) + sizeof(ui32)) * opts.BatchSize * 16;
        return storageSize + neighborsSize + (1ull << 30);
    }

    Y_SAVELOAD_JOB(Opts, DistanceTraits, ShardSizes, EstimatedItemSize);

private:
    THnswInternalYtBuildOptions Opts;
    TDistanceTraits DistanceTraits;
    THashMap<ui64, ui64> ShardSizes;
    size_t EstimatedItemSize = 0;
};

/**
 * @brief Trims all neighbors in levelsTable to opts.MaxNeighbors elements
 *
 * @param itemsTable    {shard_id:ui64 id:ui64 item:str}
 * @param levelsTable   {shard_id:ui64 level_id:ui64 id:ui64 neighbors:str}
 */
template<class TDistanceTraits,
         class TItemStorage>
void TrimNeighbors(const THnswInternalYtBuildOptions& opts,
                   const TDistanceTraits& distanceTraits,
                   NYT::IClientPtr client,
                   const TItemsTable& itemsTable,
                   const TString& levelsTable,
                   const THashMap<ui64, ui64>& shardSizes) {

    const auto itemSize = itemsTable.EstimateItemSize();
    const size_t maxShardSize = GetMaxShardSize(shardSizes);

    NYT::TUserJobSpec reducerSpec;
    const ui64 memoryLimit = opts.AdditionalMemoryLimit +
        TTrimNeighborsReducer<TDistanceTraits, TItemStorage>::EstimateMemoryLimit(opts, itemSize, maxShardSize);
    reducerSpec.MemoryLimit(memoryLimit);

    NYT::TNode jobSpec = NYT::TNode()
        ("reducer", NYT::TNode()
            ("cpu_limit", opts.CpuLimit));
    if (opts.NumTrimNeighborsJobs != NHnsw::THnswBuildOptions::AutoSelect) {
        jobSpec("job_count", shardSizes.size() * opts.NumTrimNeighborsJobs)
                ("enable_job_splitting", false);
    }

    client->JoinReduce(
        NYT::TJoinReduceOperationSpec()
            .AddInput<NYT::TNode>(itemsTable.GetPath().Foreign(true))
            .AddInput<NYT::TNode>(levelsTable)
            .AddOutput<NYT::TNode>(NYT::TRichYPath(levelsTable).SortedBy({ "shard_id", "level_id", "id" }))
            .JoinBy({ "shard_id" })
            .ReducerSpec(reducerSpec),
        new TTrimNeighborsReducer<TDistanceTraits, TItemStorage>(opts, distanceTraits, shardSizes, itemSize),
        NYT::TOperationOptions().Spec(jobSpec)
    );
}


/**
 * @brief Writes meta information necessary for reading THnswIndexData from YT.
 *
 * @param levelsTable   {shard_id:ui64 level_id:ui64 id:ui64 neighbors:str}
 */
void ConstructIndexData(const THnswInternalYtBuildOptions& opts,
                        NYT::IClientPtr client,
                        const TString& levelsTable,
                        const THashMap<ui64, ui64>& shardSizes);

}
