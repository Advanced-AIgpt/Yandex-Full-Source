#include "build_routines.h"

#include <util/generic/map.h>

namespace NHnsw {

namespace NPrivate {

class TShardSizeMapper : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    TShardSizeMapper() = default;

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        THashMap<ui64, ui64> shardSizes;
        for (; input->IsValid(); input->Next()) {
            const auto& row = input->GetRow();
            ui64 shardId = row["shard_id"].AsUint64();
            ++shardSizes[shardId];
        }
        for (const auto& pair : shardSizes) {
            ui64 shardId = pair.first;
            ui64 size = pair.second;
            output->AddRow(NYT::TNode()("shard_id", shardId)("size", size));
        }
    }
};
REGISTER_MAPPER(TShardSizeMapper);

class TShardSizeReducer : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    TShardSizeReducer() = default;

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        ui64 shardId;
        ui64 size = 0;
        for (; input->IsValid(); input->Next()) {
            const auto& row = input->GetRow();
            shardId = row["shard_id"].AsUint64();
            size += row["size"].AsUint64();
        }
        output->AddRow(NYT::TNode()("shard_id", shardId)("size", size));
    }
};
REGISTER_REDUCER(TShardSizeReducer);

static TMap<ui64, ui64> ComputeShardOffsets(NYT::IClientPtr client, const TString& shardSizesTable) {
    auto reader = client->CreateTableReader<NYT::TNode>(shardSizesTable);
    TMap<ui64, ui64> shardOffsets;
    for (; reader->IsValid(); reader->Next()) {
        ui64 shardId = reader->GetRow()["shard_id"].AsUint64();
        ui64 size = reader->GetRow()["size"].AsUint64();
        shardOffsets[shardId] = size;
    }
    ui64 offset = 0;
    for (auto& pair : shardOffsets) {
        ui64 size = pair.second;
        pair.second = offset;
        offset += size;
    }
    return shardOffsets;
}

static size_t ComputeMaxShardSize(NYT::IClientPtr client, const TString& shardSizesTable) {
    auto reader = client->CreateTableReader<NYT::TNode>(shardSizesTable);
    ui64 maxSize = 0;
    for (; reader->IsValid(); reader->Next()) {
        ui64 size = reader->GetRow()["size"].AsUint64();
        maxSize = Max(maxSize, size);
    }
    return maxSize;
}

class TEnumerateMapper : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    TEnumerateMapper() = default;

    explicit TEnumerateMapper(const TMap<ui64, ui64>& shardOffsets)
        : ShardOffsets(shardOffsets)
    {
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        for (; input->IsValid(); input->Next()) {
            const auto& row = input->GetRow();
            ui64 shardId = row["shard_id"].AsUint64();
            Y_VERIFY(input->GetRowIndex() >= ShardOffsets[shardId]);
            NYT::TNode copy = row;
            copy["id"] = input->GetRowIndex() - ShardOffsets[shardId];
            output->AddRow(copy);
        }
    }

    Y_SAVELOAD_JOB(ShardOffsets);
private:
    TMap<ui64, ui64> ShardOffsets;
};
REGISTER_MAPPER(TEnumerateMapper);

size_t EnumerateItems(NYT::IClientPtr client, const TString& inputTable, const TString& itemsTable) {
    NYT::TTempTable shardSizesTable(client);
    client->MapReduce(
        NYT::TMapReduceOperationSpec()
            .AddInput<NYT::TNode>(inputTable)
            .AddOutput<NYT::TNode>(shardSizesTable.Name())
            .ReduceBy({ "shard_id" }),
        new TShardSizeMapper,
        new TShardSizeReducer);

    auto shardOffsets = ComputeShardOffsets(client, shardSizesTable.Name());
    ui64 maxShardSize = ComputeMaxShardSize(client, shardSizesTable.Name());

    client->Sort(
        NYT::TSortOperationSpec()
            .AddInput(inputTable)
            .Output(itemsTable)
            .SortBy({ "shard_id", "doc_id" }));
    client->Map(
        NYT::TMapOperationSpec()
            .AddInput<NYT::TNode>(itemsTable)
            .AddOutput<NYT::TNode>(itemsTable)
            .Ordered(true),
        new TEnumerateMapper(shardOffsets));
    client->Sort(
        NYT::TSortOperationSpec()
            .AddInput(itemsTable)
            .Output(itemsTable)
            .SortBy({ "id", "shard_id" }));

    return maxShardSize;
}

class TInitFromPreviousLevelMapper : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    TInitFromPreviousLevelMapper() = default;

    explicit TInitFromPreviousLevelMapper(size_t levelId)
        : LevelId(levelId)
    {
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        for (; input->IsValid(); input->Next()) {
            const auto& row = input->GetRow();
            output->AddRow(row);
            if (row["level_id"].AsUint64() != LevelId + 1) {
                continue;
            }
            NYT::TNode copy = row;
            copy["level_id"] = LevelId;
            output->AddRow(copy);
        }
    }

    Y_SAVELOAD_JOB(LevelId);
private:
    size_t LevelId;
};
REGISTER_MAPPER(TInitFromPreviousLevelMapper);

void InitFromPreviousLevel(NYT::IClientPtr client, size_t levelId, const TString& levelsTable) {
    client->Map(
        NYT::TMapOperationSpec()
            .AddInput<NYT::TNode>(levelsTable)
            .AddOutput<NYT::TNode>(levelsTable),
        new TInitFromPreviousLevelMapper(levelId));
    client->Sort(
        NYT::TSortOperationSpec()
            .AddInput(levelsTable)
            .Output(levelsTable)
            .SortBy({ "shard_id", "level_id", "id" }));
}

}

}
