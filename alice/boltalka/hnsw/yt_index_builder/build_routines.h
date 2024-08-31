#pragma once
#include "internal_build_options.h"

#include <library/cpp/hnsw/index_builder/index_builder.h>

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/operation.h>
#include <mapreduce/yt/util/temp_table.h>

#include <util/generic/vector.h>
#include <util/generic/deque.h>
#include <util/ysaveload.h>

namespace NHnsw {

namespace NPrivate {

size_t EnumerateItems(NYT::IClientPtr client, const TString& inputTable, const TString& itemsTable);
void InitFromPreviousLevel(NYT::IClientPtr client, size_t levelId, const TString& levelsTable);

template<class TNeighbors>
void ReadNeighbors(const NYT::TNode& row, TNeighbors* neighbors) {
    if (!row.HasKey("neighbors")) {
        return;
    }
    using TNeighbor = typename TNeighbors::value_type;
    TString buf = row["neighbors"].AsString();
    const auto* neighborsBegin = reinterpret_cast<const TNeighbor*>(buf.data());
    const auto* neighborsEnd = reinterpret_cast<const TNeighbor*>(buf.data() + buf.size());
    neighbors->insert(neighbors->end(), neighborsBegin, neighborsEnd);
}

template<class TNeighbors>
void WriteNeighbors(const TNeighbors& neighbors, NYT::TNode* row) {
    const auto* neighborsBegin = reinterpret_cast<const char*>(neighbors.data());
    const auto* neighborsEnd = reinterpret_cast<const char*>(neighbors.data() + neighbors.size());
    (*row)["neighbors"] = TString(neighborsBegin, neighborsEnd);
}

template<class TLevels>
void ReadLevels(NYT::TTableReader<NYT::TNode>* input, size_t tableIndex, TLevels* levels) {
    levels->clear();
    size_t prevLevelId = Max<size_t>();
    for (; input->IsValid() && input->GetTableIndex() == tableIndex; input->Next()) {
        const auto& row = input->GetRow();
        size_t levelId = row["level_id"].AsUint64();
        if (levelId != prevLevelId) {
            levels->emplace_back();
        }
        Y_VERIFY(levels->back().size() == row["id"].AsUint64());
        levels->back().emplace_back();
        ReadNeighbors(row, &levels->back().back());
        prevLevelId = levelId;
    }
}

template<class TItemStorage>
void ReadItems(NYT::TTableReader<NYT::TNode>* input, size_t tableIndex, TItemStorage* itemStorage) {
    for (; input->IsValid() && input->GetTableIndex() == tableIndex; input->Next()) {
        const auto& row = input->GetRow();
        itemStorage->AddItemFromString(row["item"].AsString());
    }
    itemStorage->Finalize();
}

}

template<class TNeighbors>
class TConstructIndexDataMapper : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    TConstructIndexDataMapper() = default;

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        for (; input->IsValid(); input->Next()) {
            const auto& row = input->GetRow();
            TNeighbors neighbors;
            NPrivate::ReadNeighbors(row, &neighbors);
            TVector<ui32> ids;
            ids.reserve(neighbors.size());
            for (const auto& n : neighbors) {
                ids.push_back(n.Id);
            }
            const auto* idsBegin = reinterpret_cast<const char*>(ids.data());
            const auto* idsEnd = reinterpret_cast<const char*>(ids.data() + ids.size());
            NYT::TNode result = row;
            result["neighbors"] = TString(idsBegin, idsEnd);
            output->AddRow(result);
        }
    }
};

template<class TNeighbors>
void ConstructIndexData(const THnswInternalYtBuildOptions& opts,
                        size_t numItems,
                        NYT::IClientPtr client,
                        const TString& levelsTable) {
    client->Map(
        NYT::TMapOperationSpec()
            .AddInput<NYT::TNode>(levelsTable)
            .AddOutput<NYT::TNode>(NYT::TRichYPath(levelsTable).SortedBy({ "shard_id", "level_id", "id" }))
            .Ordered(true),
        new TConstructIndexDataMapper<TNeighbors>);

    client->Set(levelsTable + "/@num_items", numItems);
    client->Set(levelsTable + "/@max_neighbors", opts.MaxNeighbors);
    client->Set(levelsTable + "/@level_size_decay", opts.LevelSizeDecay);
}

template<class TDistanceTraits,
         class TItemStorage>
class TFindApproximateNeighborsReducer : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
    using TItem = typename TItemStorage::TItem;
    using TNeighbors = typename TDistanceTraits::TNeighbors;
    using TLevels = typename TDistanceTraits::TLevels;
public:
    TFindApproximateNeighborsReducer() = default;

    TFindApproximateNeighborsReducer(const THnswInternalYtBuildOptions& opts,
                                     const TDistanceTraits& distanceTraits)
        : Opts(opts)
        , DistanceTraits(distanceTraits)
    {
    }

    void Start(NYT::TTableWriter<NYT::TNode>*) override {
        NPar::LocalExecutor().RunAdditionalThreads(Opts.NumThreads - 1);
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        TLevels levels;
        NPrivate::ReadLevels(input, /*tableIndex*/ 0, &levels);

        TItemStorage itemStorage(Opts.AnnSearchItemStorageMemoryLimit);
        NPrivate::ReadItems(input, /*tableIndex*/ 1, &itemStorage);

        TVector<NYT::TNode> queries;
        for (; input->IsValid(); input->Next()) {
            queries.push_back(input->GetRow());
        }
        auto task = [&](int id) {
            TString queryStr = queries[id]["item"].AsString();
            const auto& query = TItemStorage::ParseItemFromString(queryStr);
            TNeighbors neighbors;
            FindApproximateNeighbors(Opts, DistanceTraits, itemStorage, levels, query, &neighbors);
            TrimNeighbors(Opts, DistanceTraits, itemStorage, &neighbors);
            NPrivate::WriteNeighbors(neighbors, &queries[id]);
        };
        NPar::LocalExecutor().ExecRange(task, 0, queries.size(), NPar::TLocalExecutor::WAIT_COMPLETE);

        for (const auto& row : queries) {
            output->AddRow(row);
        }
    }

    Y_SAVELOAD_JOB(Opts, DistanceTraits);

private:
    void FindApproximateNeighbors(const THnswInternalBuildOptions& opts,
                                  const TDistanceTraits& distanceTraits,
                                  const TItemStorage& itemStorage,
                                  const TLevels& levels,
                                  const TItem& query,
                                  TNeighbors* result) {
        using TNeighborMaxQueue = typename TDistanceTraits::TNeighborMaxQueue;
        using TNeighborMinQueue = typename TDistanceTraits::TNeighborMinQueue;

        size_t entryId = 0;
        auto entryDist = distanceTraits.Distance(query, itemStorage.GetItem(entryId));
        for (size_t level = levels.size(); level-- > 1;) {
            for (bool entryChanged = true; entryChanged;) {
                entryChanged = false;
                for (const auto neighbor: levels[level][entryId]) {
                    auto distToQuery = distanceTraits.Distance(query, itemStorage.GetItem(neighbor.Id));
                    if (distanceTraits.DistanceLess(distToQuery, entryDist)) {
                        entryDist = distToQuery;
                        entryId = neighbor.Id;
                        entryChanged = true;
                    }
                }
            }
        }

        TNeighborMaxQueue nearest(distanceTraits.NeighborLess);
        TNeighborMinQueue candidates(distanceTraits.NeighborGreater);
        TDenseHashSet<size_t> visited(~size_t(0));
        nearest.push({entryDist, entryId});
        candidates.push({entryDist, entryId});
        visited.Insert(entryId);

        const auto& thisLevel = levels[0];

        while (!candidates.empty()) {
            auto cur = candidates.top();
            candidates.pop();
            if (distanceTraits.DistanceLess(nearest.top().Dist, cur.Dist)) {
                break;
            }
            for (const auto& neighbor: thisLevel[cur.Id]) {
                if (visited.Has(neighbor.Id)) {
                    continue;
                }
                auto distToQuery = distanceTraits.Distance(query, itemStorage.GetItem(neighbor.Id));
                if (nearest.size() < opts.SearchNeighborhoodSize || distanceTraits.DistanceLess(distToQuery, nearest.top().Dist)) {
                    nearest.push({distToQuery, neighbor.Id});
                    candidates.push({distToQuery, neighbor.Id});
                    visited.Insert(neighbor.Id);
                    if (nearest.size() > opts.SearchNeighborhoodSize) {
                        nearest.pop();
                    }
                }
            }
        }

        for (; !nearest.empty(); nearest.pop()) {
            result->push_back(nearest.top());
        }
    }

    THnswInternalYtBuildOptions Opts;
    TDistanceTraits DistanceTraits;
};

template<class TDistanceTraits,
         class TItemStorage>
void BuildApproximateNeighbors(const THnswInternalYtBuildOptions& opts,
                               const TDistanceTraits& distanceTraits,
                               NYT::IClientPtr client,
                               const TString& itemsTable,
                               const TString& batchTable,
                               const TString& levelsTable) {
    NYT::TUserJobSpec reducerSpec;
    if (opts.AnnSearchMemoryLimit != THnswBuildOptions::AutoSelect) {
        reducerSpec.MemoryLimit(opts.AnnSearchMemoryLimit);
    }
    NYT::TNode jobSpec;
    if (opts.AnnSearchJobCount != THnswBuildOptions::AutoSelect) {
        jobSpec["job_count"] = opts.AnnSearchJobCount;
    }
    jobSpec["cpu_limit"] = opts.NumThreads;

    client->JoinReduce(
        NYT::TJoinReduceOperationSpec()
            .AddInput<NYT::TNode>(NYT::TRichYPath(levelsTable).Foreign(true))
            .AddInput<NYT::TNode>(NYT::TRichYPath(itemsTable).Foreign(true))
            .AddInput<NYT::TNode>(batchTable)
            .AddOutput<NYT::TNode>(NYT::TRichYPath(batchTable).SortedBy({ "shard_id", "id" }))
            .JoinBy({ "shard_id" })
            .ReducerSpec(reducerSpec),
        new TFindApproximateNeighborsReducer<TDistanceTraits, TItemStorage>(opts, distanceTraits),
        NYT::TOperationOptions().Spec(jobSpec));
}

template<class TDistanceTraits,
         class TItemStorage>
class TAddExactNeighborsReducer : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
    using TItem = typename TItemStorage::TItem;
    using TNeighbors = typename TDistanceTraits::TNeighbors;
public:
    TAddExactNeighborsReducer() = default;

    TAddExactNeighborsReducer(const THnswInternalYtBuildOptions& opts,
                              const TDistanceTraits& distanceTraits)
        : Opts(opts)
        , DistanceTraits(distanceTraits)
    {
    }

    void Start(NYT::TTableWriter<NYT::TNode>*) override {
        NPar::LocalExecutor().RunAdditionalThreads(Opts.NumThreads - 1);
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        const size_t batchBegin = input->GetRow()["id"].AsUint64();

        TItemStorage batchItems(Opts.ExactSearchItemStorageMemoryLimit);
        NPrivate::ReadItems(input, /*tableIndex*/ 0, &batchItems);

        TVector<NYT::TNode> queries;
        for (; input->IsValid(); input->Next()) {
            queries.push_back(input->GetRow());
        }
        auto task = [&](int id) {
            TString queryStr = queries[id]["item"].AsString();
            const auto& query = TItemStorage::ParseItemFromString(queryStr);
            size_t queryIdInBatch = queries[id]["id"].AsUint64() - batchBegin;
            Y_VERIFY(queries[id]["id"].AsUint64() >= batchBegin);

            TNeighbors neighbors;
            NPrivate::ReadNeighbors(queries[id], &neighbors);
            size_t prevNeighborsSize = neighbors.size();
            FindExactNeighborsInBatch(Opts, DistanceTraits, batchItems, 0, batchItems.GetNumItems(), queryIdInBatch, query, &neighbors);
            for (size_t i = prevNeighborsSize; i < neighbors.size(); ++i) {
                neighbors[i].Id += batchBegin;
            }
            NPrivate::WriteNeighbors(neighbors, &queries[id]);
        };
        NPar::LocalExecutor().ExecRange(task, 0, queries.size(), NPar::TLocalExecutor::WAIT_COMPLETE);

        for (const auto& row : queries) {
            output->AddRow(row);
        }
    }

    Y_SAVELOAD_JOB(Opts, DistanceTraits);

private:
    THnswInternalYtBuildOptions Opts;
    TDistanceTraits DistanceTraits;
};

template<class TDistanceTraits,
         class TItemStorage>
void AddExactNeighborsInBatch(const THnswInternalYtBuildOptions& opts,
                              const TDistanceTraits& distanceTraits,
                              NYT::IClientPtr client,
                              const TString& batchTable) {
    NYT::TUserJobSpec reducerSpec;
    if (opts.ExactSearchMemoryLimit != THnswBuildOptions::AutoSelect) {
        reducerSpec.MemoryLimit(opts.ExactSearchMemoryLimit);
    }
    NYT::TNode jobSpec;
    if (opts.ExactSearchJobCount != THnswBuildOptions::AutoSelect) {
        jobSpec["job_count"] = opts.ExactSearchJobCount;
    }
    jobSpec["cpu_limit"] = opts.NumThreads;

    client->JoinReduce(
        NYT::TJoinReduceOperationSpec()
            .AddInput<NYT::TNode>(NYT::TRichYPath(batchTable).Foreign(true))
            .AddInput<NYT::TNode>(batchTable)
            .AddOutput<NYT::TNode>(NYT::TRichYPath(batchTable).SortedBy({ "shard_id", "id" }))
            .JoinBy({ "shard_id" })
            .ReducerSpec(reducerSpec),
        new TAddExactNeighborsReducer<TDistanceTraits, TItemStorage>(opts, distanceTraits),
        NYT::TOperationOptions().Spec(jobSpec));
}

template<class TNeighbors>
class TAppendBatchToLevelsMapper : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    TAppendBatchToLevelsMapper() = default;

    explicit TAppendBatchToLevelsMapper(size_t levelId, size_t batchBegin)
        : LevelId(levelId)
        , BatchBegin(batchBegin)
    {
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        for (; input->IsValid(); input->Next()) {
            const auto& row = input->GetRow();
            NYT::TNode copy = row;
            copy["level_id"] = LevelId;
            output->AddRow(copy);

            size_t curId = row["id"].AsUint64();

            TNeighbors neighbors;
            NPrivate::ReadNeighbors(row, &neighbors);
            for (const auto& n : neighbors) {
                if (n.Id >= BatchBegin) {
                    continue;
                }
                NYT::TNode result = NYT::TNode::CreateMap();
                result["shard_id"] = row["shard_id"].AsUint64();
                result["level_id"] = LevelId;
                result["id"] = n.Id;
                NPrivate::WriteNeighbors<TNeighbors>({ {n.Dist, curId} }, &result);
                output->AddRow(result);
            }
        }
    }

    Y_SAVELOAD_JOB(LevelId, BatchBegin);
private:
    size_t LevelId;
    size_t BatchBegin;
};

template<class TNeighbors>
class TJoinNeighborsReducer : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    TJoinNeighborsReducer() = default;

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        NYT::TNode mainRow = NYT::TNode::CreateMap();
        TNeighbors neighbors;
        for (; input->IsValid(); input->Next()) {
            const auto& row = input->GetRow();
            NPrivate::ReadNeighbors(row, &neighbors);
            if (row.HasKey("doc_id")) {
                Y_VERIFY(mainRow.Empty());
                mainRow = row;
            }
        }
        Y_VERIFY(!mainRow.Empty());
        NPrivate::WriteNeighbors(neighbors, &mainRow);
        output->AddRow(mainRow);
    }
};

template<class TDistanceTraits>
void UpdatePreviousNeighbors(size_t levelId,
                             size_t batchBegin,
                             NYT::IClientPtr client,
                             const TString& batchTable,
                             const TString& levelsTable) {
    using TNeighbors = typename TDistanceTraits::TNeighbors;
    client->Map(
        NYT::TMapOperationSpec()
            .AddInput<NYT::TNode>(NYT::TRichYPath(batchTable)
                .Columns({ "shard_id", "level_id", "id", "doc_id", "neighbors" }))
            .AddOutput<NYT::TNode>(NYT::TRichYPath(levelsTable).Append(true)),
        new TAppendBatchToLevelsMapper<TNeighbors>(levelId, batchBegin));
    client->Sort(
        NYT::TSortOperationSpec()
            .AddInput(levelsTable)
            .Output(levelsTable)
            .SortBy({ "shard_id", "level_id", "id" }));
    client->Reduce(
        NYT::TReduceOperationSpec()
            .AddInput<NYT::TNode>(levelsTable)
            .AddOutput<NYT::TNode>(NYT::TRichYPath(levelsTable).SortedBy({ "shard_id", "level_id", "id" }))
            .ReduceBy({ "shard_id", "level_id", "id" }),
        new TJoinNeighborsReducer<TNeighbors>);
}

template<class TDistanceTraits,
         class TItemStorage>
class TTrimNeighborsReducer : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
    using TItem = typename TItemStorage::TItem;
    using TNeighbors = typename TDistanceTraits::TNeighbors;
public:
    TTrimNeighborsReducer() = default;

    TTrimNeighborsReducer(const THnswInternalYtBuildOptions& opts,
                          const TDistanceTraits& distanceTraits,
                          size_t levelId)
        : Opts(opts)
        , DistanceTraits(distanceTraits)
        , LevelId(levelId)
    {
    }

    void Start(NYT::TTableWriter<NYT::TNode>*) override {
        NPar::LocalExecutor().RunAdditionalThreads(Opts.NumThreads - 1);
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        TItemStorage itemStorage(Opts.AnnSearchItemStorageMemoryLimit);
        NPrivate::ReadItems(input, /*tableIndex*/ 0, &itemStorage);

        TVector<NYT::TNode> queries;
        for (; input->IsValid(); input->Next()) {
            queries.push_back(input->GetRow());
        }
        auto task = [&](int id) {
            if (queries[id]["level_id"].AsUint64() != LevelId) {
                return;
            }
            TNeighbors neighbors;
            NPrivate::ReadNeighbors(queries[id], &neighbors);
            if (neighbors.size() <= Opts.MaxNeighbors) {
                return;
            }
            TrimNeighbors(Opts, DistanceTraits, itemStorage, &neighbors);
            NPrivate::WriteNeighbors(neighbors, &queries[id]);
        };
        NPar::LocalExecutor().ExecRange(task, 0, queries.size(), NPar::TLocalExecutor::WAIT_COMPLETE);

        for (const auto& row : queries) {
            output->AddRow(row);
        }
    }

    Y_SAVELOAD_JOB(Opts, DistanceTraits, LevelId);

private:
    THnswInternalYtBuildOptions Opts;
    TDistanceTraits DistanceTraits;
    size_t LevelId;
};

template<class TDistanceTraits,
         class TItemStorage>
void TrimNeighbors(const THnswInternalYtBuildOptions& opts,
                   const TDistanceTraits& distanceTraits,
                   size_t levelId,
                   NYT::IClientPtr client,
                   const TString& itemsTable,
                   const TString& levelsTable) {
    // Reusing options from ANN search, as the pattern of computation is very similair
    NYT::TUserJobSpec reducerSpec;
    if (opts.AnnSearchMemoryLimit != THnswBuildOptions::AutoSelect) {
        reducerSpec.MemoryLimit(opts.AnnSearchMemoryLimit);
    }
    NYT::TNode jobSpec;
    if (opts.AnnSearchJobCount != THnswBuildOptions::AutoSelect) {
        jobSpec["job_count"] = opts.AnnSearchJobCount;
    }

    client->JoinReduce(
        NYT::TJoinReduceOperationSpec()
            .AddInput<NYT::TNode>(NYT::TRichYPath(itemsTable).Foreign(true))
            .AddInput<NYT::TNode>(levelsTable)
            .AddOutput<NYT::TNode>(NYT::TRichYPath(levelsTable).SortedBy({ "shard_id", "level_id", "id" }))
            .JoinBy({ "shard_id" })
            .ReducerSpec(reducerSpec),
        new TTrimNeighborsReducer<TDistanceTraits, TItemStorage>(opts, distanceTraits, levelId),
        NYT::TOperationOptions().Spec(jobSpec));
}

template<class TDistanceTraits,
         class TItemStorage>
void ProcessBatch(const THnswInternalYtBuildOptions& opts,
                  const TDistanceTraits& distanceTraits,
                  size_t levelId,
                  size_t batchBegin,
                  size_t batchEnd,
                  NYT::IClientPtr client,
                  const TString& itemsTable,
                  const TString& levelsTable) {

    NYT::TTempTable curItemsTable(client);
    client->Sort(
        NYT::TSortOperationSpec()
            .AddInput(NYT::TRichYPath(itemsTable).AddRange(NYT::TReadRange()
                .LowerLimit(NYT::TReadLimit().Key(0))
                .UpperLimit(NYT::TReadLimit().Key(batchEnd))))
            .Output(curItemsTable.Name())
            .SortBy({ "shard_id", "id" }));

    NYT::TTempTable batchTable(client);
    client->Sort(
        NYT::TSortOperationSpec()
            .AddInput(NYT::TRichYPath(itemsTable).AddRange(NYT::TReadRange()
                .LowerLimit(NYT::TReadLimit().Key(batchBegin))
                .UpperLimit(NYT::TReadLimit().Key(batchEnd))))
            .Output(batchTable.Name())
            .SortBy({ "shard_id", "id" }));

    THPTimer watch;
    if (batchBegin > 0) {
        BuildApproximateNeighbors<TDistanceTraits, TItemStorage>(opts, distanceTraits, client, curItemsTable.Name(), batchTable.Name(), levelsTable);
        if (opts.Verbose) {
            Cerr << "\tbuild ann " << watch.PassedReset() / (batchEnd - batchBegin) << Endl;
        }
    }
    AddExactNeighborsInBatch<TDistanceTraits, TItemStorage>(opts, distanceTraits, client, batchTable.Name());
    if (opts.Verbose) {
        Cerr << "\tbuild exact " << watch.PassedReset() / (batchEnd - batchBegin) << Endl;
    }
    UpdatePreviousNeighbors<TDistanceTraits>(levelId, batchBegin, client, batchTable.Name(), levelsTable);
    if (opts.Verbose) {
        Cerr << "\tbuild prev " << watch.PassedReset() / (batchEnd - batchBegin) << Endl;
    }
    TrimNeighbors<TDistanceTraits, TItemStorage>(opts, distanceTraits, levelId, client, curItemsTable.Name(), levelsTable);
    if (opts.Verbose) {
        Cerr << "\ttrim neighbors " << watch.PassedReset() / (batchEnd - batchBegin) << Endl;
    }
}

template<class TDistanceTraits,
         class TItemStorage>
void BuildLevel(const THnswInternalYtBuildOptions& opts,
                const TDistanceTraits& distanceTraits,
                size_t prevLevelSize,
                size_t levelId,
                size_t levelSize,
                size_t batchSize,
                NYT::IClientPtr client,
                const TString& itemsTable,
                const TString& levelsTable) {

    size_t builtLevelSize = 0;
    if (prevLevelSize >= batchSize) {
        builtLevelSize = prevLevelSize;
        NPrivate::InitFromPreviousLevel(client, levelId, levelsTable);
    }

    for (size_t batchBegin = builtLevelSize; batchBegin < levelSize; ) {
        const size_t curBatchSize = Min(levelSize - batchBegin, batchSize);
        const size_t batchEnd = batchBegin + curBatchSize;
        ProcessBatch<TDistanceTraits, TItemStorage>(opts, distanceTraits, levelId, batchBegin, batchEnd, client, itemsTable, levelsTable);
        batchBegin = batchEnd;
        Cerr << batchEnd * 100 / levelSize << "% done\r";
    }
}

}
