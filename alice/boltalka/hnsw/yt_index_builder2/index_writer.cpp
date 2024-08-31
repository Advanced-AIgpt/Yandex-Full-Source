#include "index_writer.h"

#include <util/generic/string.h>
#include <util/stream/file.h>
#include <util/string/cast.h>

namespace NYtHnsw {

namespace {

size_t WriteShardInformation(const THnswYtIndexData& indexData, ui64 shardId, IOutputStream& out) {
    const ui32 numItems = indexData.ShardNumItems.at(shardId);
    out.Write(&numItems, sizeof(numItems));
    out.Write(&indexData.MaxNeighbors, sizeof(indexData.MaxNeighbors));
    out.Write(&indexData.LevelSizeDecay, sizeof(indexData.LevelSizeDecay));
    return sizeof(numItems) + sizeof(indexData.MaxNeighbors) + sizeof(indexData.LevelSizeDecay);
}

size_t ReadLevel(NYT::TTableReaderPtr<NYT::TNode> input, size_t maxNeighbours, size_t shardId, IOutputStream& out, size_t& levelSize) {
    const ui64 levelId = input->GetRow()["level_id"].AsUint64();
    levelSize = 0;
    size_t currentSize = 0;
    size_t numNeighbors = 0;
    for (; input->IsValid() && input->GetRow()["level_id"].AsUint64() == levelId && input->GetRow()["shard_id"].AsUint64() == shardId; input->Next()) {
        const auto& neighbors = input->GetRow()["neighbors"].AsString();
        out.Write(neighbors.data(), neighbors.size());
        currentSize += neighbors.size();

        Y_ENSURE(numNeighbors == 0 || neighbors.size() == numNeighbors * sizeof(ui32));
        numNeighbors = neighbors.size() / sizeof(ui32);
        ++levelSize;
    }
    Y_ENSURE(numNeighbors == Min<size_t>(levelSize - 1, maxNeighbours));
    return currentSize;
}

}

void WriteAllIndexes(NYT::IClientPtr client, const TString& inputTable, IOutputStream& out, TVector<ui64>& offsets) {
    const THnswYtIndexData indexData = LoadIndexDataFromTableAttribute(client, inputTable);
    auto input = client->CreateTableReader<NYT::TNode>(NYT::TRichYPath(inputTable));

    offsets.reserve(indexData.ShardNumItems.size());
    size_t expectedLevelSize;
    size_t shardId;
    size_t fileSize = 0;
    bool writingStarted = false;
    while(input->IsValid()) {
        const auto& row = input->GetRow();
        if (!writingStarted || row["shard_id"].AsUint64() != shardId){
            writingStarted = true;
            offsets.push_back(fileSize);

            shardId = row["shard_id"].AsUint64();
            fileSize += WriteShardInformation(indexData, shardId, out);
            expectedLevelSize = indexData.ShardNumItems.at(shardId);
        }

        size_t levelSize;
        fileSize += ReadLevel(input, indexData.MaxNeighbors, shardId, out, levelSize);
        Y_ENSURE(levelSize == expectedLevelSize);
        expectedLevelSize /= indexData.LevelSizeDecay;
    }
}

void WriteIndex(const THnswYtIndexData& indexData, NYT::TTableReader<NYT::TNode>* input, size_t shardId, IOutputStream& out) {
    WriteShardInformation(indexData, shardId, out);
    const ui32 numItems = indexData.ShardNumItems.at(shardId);

    size_t expectedLevelSize = numItems;
    while (input->IsValid()) {
        size_t levelSize;
        ReadLevel(input, indexData.MaxNeighbors, shardId, out, levelSize);
        Y_ENSURE(levelSize == expectedLevelSize);
        expectedLevelSize /= indexData.LevelSizeDecay;
    }
}

void WriteIndex(NYT::IClientPtr client, const TString& indexDataTable, size_t shardId, IOutputStream& out) {
    const THnswYtIndexData indexData = LoadIndexDataFromTableAttribute(client, indexDataTable);
    auto input = client->CreateTableReader<NYT::TNode>(
        NYT::TRichYPath(indexDataTable).AddRange(NYT::TReadRange().Exact(NYT::TReadLimit()
            .Key(shardId))));

    WriteIndex(indexData, input.Get(), shardId, out);
}

void WriteIndex(NYT::IClientPtr client, const TString& indexDataTable, size_t shardId, const TString& outputFilename) {
    TFixedBufferFileOutput out(outputFilename);
    WriteIndex(client, indexDataTable, shardId, out);
}

void WriteAllIndexes(NYT::IClientPtr client, const TString& inputTable, const TString& outputFilename, const TString& offsetsFilename) {
    TFixedBufferFileOutput out(outputFilename);
    TVector<ui64> offsets;
    WriteAllIndexes(client, inputTable, out, offsets);
    TFixedBufferFileOutput offsetsFile(offsetsFilename);
    offsetsFile.Write(offsets.data(), offsets.size() * sizeof(ui64));
}

}

