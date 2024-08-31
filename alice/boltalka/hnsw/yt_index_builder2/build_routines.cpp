#include "build_routines.h"
#include "index_data.h"

#include <mapreduce/yt/library/table_schema/protobuf.h>

#include <util/generic/map.h>
#include <util/string/cast.h>

#include <limits>

namespace NYtHnsw {

namespace {
class TEnumerateItemsReducer : public NYT::IReducer<NYT::TNodeReader, NYT::TMessageWriter> {
public:
    struct OutputTables {
        enum Index {
            Items,
            Sizes
        };
    };

    void Do(TReader* reader, TWriter* writer) override {
        Y_ENSURE(reader->IsValid());
        const auto shardId = reader->GetRow()["shard_id"].AsUint64();

        Y_ENSURE(shardId <= std::numeric_limits<uint32_t>::max(), "shard_id can't exceed UInt32");

        ui64 count = 0;
        {
            TEmbeddingPB item;
            item.SetShardId(shardId);
            for (; reader->IsValid(); reader->Next()) {
                const auto& row = reader->GetRow();
                item.SetItemId(count++);
                item.SetDocId(row["doc_id"].AsUint64());
                item.SetItem(row["item"].AsString());
                writer->AddRow(item, OutputTables::Items);
            }
        }
        {
            TShardSizePB size;
            size.SetShardId(shardId);
            size.SetSize(count);
            writer->AddRow(size, OutputTables::Sizes);
        }
    }
};

REGISTER_REDUCER(TEnumerateItemsReducer)

class TEnumeratePartitionsMapper : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    TEnumeratePartitionsMapper() = default;

    TEnumeratePartitionsMapper(const THnswInternalYtBuildOptions& opts,
                               const THashMap<ui64, ui64>& shardSizes,
                               size_t numPartitions,
                               size_t levelId)
        : Opts(opts)
        , ShardSizes(shardSizes)
        , NumPartitions(numPartitions)
        , LevelId(levelId)
    {
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        TVector<size_t> levelSizes;
        ui64 prevShardId = Max<ui64>();
        for (; input->IsValid(); input->Next()) {
            const auto& row = input->GetRow();
            const ui64 shardId = row["shard_id"].AsUint64();

            if (levelSizes.empty() || prevShardId != shardId) {
                levelSizes = NHnsw::GetLevelSizes(ShardSizes[shardId], Opts.LevelSizeDecay);
                prevShardId = shardId;
            }
            if (LevelId >= levelSizes.size()) {
                continue;
            }
            const ui64 id = row["id"].AsUint64();
            if (id >= levelSizes[LevelId]) {
                continue;
            }

            const ui64 partSize = (levelSizes[LevelId] + NumPartitions - 1) / NumPartitions;
            const ui64 partId = id / partSize;

            NYT::TNode copy = row;
            copy["part_id"] = partId;
            copy["part_size"] = partSize;
            output->AddRow(copy);
        }
    }

    Y_SAVELOAD_JOB(Opts, ShardSizes, NumPartitions, LevelId);
private:
    THnswInternalYtBuildOptions Opts;
    THashMap<ui64, ui64> ShardSizes;
    size_t NumPartitions;
    size_t LevelId;
};
REGISTER_MAPPER(TEnumeratePartitionsMapper);

}

size_t GetMaxShardSize(const THashMap<ui64, ui64>& shardSizes) {
    ui64 maxShardSize = 0;
    for (const auto& pair : shardSizes) {
        ui64 shardSize = pair.second;
        maxShardSize = Max(maxShardSize, shardSize);
    }
    return maxShardSize;
}

bool CheckShardSizes(const THnswInternalYtBuildOptions& opts, const THashMap<ui64, ui64>& shardSizes) {
    for (const auto& pair : shardSizes) {
        ui64 size = pair.second;
        if (size <= opts.MaxNeighbors) {
            return false;
        }
    }
    return true;
}

THashMap<ui64, ui64> EnumerateShardItems(NYT::IClientPtr client,
                                         const TString& inputTable,
                                         const TItemsTable& itemsTable) {
    THashMap<ui64, ui64> shardSizes;
    const auto tx = client->StartTransaction();
    {
        NYT::TTempTable shardSizesTable(tx);
        {
            NYT::TReduceOperationSpec spec;
            spec.AddInput<NYT::TNode>(NYT::TRichYPath(inputTable).Columns({"shard_id", "doc_id", "item"}));
            spec.SetOutput<TEmbeddingPB>(TEnumerateItemsReducer::OutputTables::Items,
                                         NYT::TRichYPath(itemsTable.GetPath())
                                         .Schema(NYT::CreateTableSchema<TEmbeddingPB>({"shard_id", "id"}))
                                         .OptimizeFor(NYT::OF_SCAN_ATTR));
            spec.SetOutput<TShardSizePB>(TEnumerateItemsReducer::OutputTables::Sizes, shardSizesTable.Name());
            spec.ReduceBy({"shard_id"});
            tx->Reduce(spec, new TEnumerateItemsReducer());
        }
        for (auto reader = tx->CreateTableReader<TShardSizePB>(shardSizesTable.Name()); reader->IsValid(); reader->Next()) {
            const auto& row = reader->GetRow();
            shardSizes[row.GetShardId()] = row.GetSize();
        }
    }
    tx->Commit();
    return shardSizes;
}

void MakeDocIdMapping(NYT::IClientPtr client,
                      const TItemsTable& itemsTable,
                      const TString& mappingTable) {
    NYT::TMergeOperationSpec spec;
    spec.AddInput(itemsTable.GetPath().Columns({"shard_id", "id", "doc_id"}));
    spec.Output(NYT::TRichYPath(mappingTable).SortedBy({"shard_id", "id"}).OptimizeFor(NYT::OF_SCAN_ATTR));
    spec.Mode(NYT::MM_ORDERED);
    client->Merge(spec);
}

void EnumeratePartitions(const THnswInternalYtBuildOptions& opts,
                         NYT::IClientPtr client,
                         const TItemsTable& itemsTable,
                         const TItemsTable& partitionedItemsTable,
                         const THashMap<ui64, ui64>& shardSizes,
                         size_t numPartitions,
                         size_t levelId) {
    client->Map(
        NYT::TMapOperationSpec()
            .AddInput<NYT::TNode>(itemsTable.GetPath())
            .AddOutput<NYT::TNode>(partitionedItemsTable.GetPath().SortedBy({ "shard_id", "part_id", "id" }))
            .Ordered(true),
        new TEnumeratePartitionsMapper(opts, shardSizes, numPartitions, levelId));
}

REGISTER_REDUCER(TJoinNeighborsReducer);

void ConstructIndexData(const THnswInternalYtBuildOptions& opts,
                        NYT::IClientPtr client,
                        const TString& levelsTable,
                        const THashMap<ui64, ui64>& shardSizes) {
    THnswYtIndexData indexData;
    indexData.ShardNumItems = shardSizes;
    indexData.MaxNeighbors = opts.MaxNeighbors;
    indexData.LevelSizeDecay = opts.LevelSizeDecay;
    SaveIndexDataToTableAttribute(client, indexData, levelsTable);
}

}

