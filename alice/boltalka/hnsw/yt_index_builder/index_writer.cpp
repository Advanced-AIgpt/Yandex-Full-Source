#include "index_writer.h"

#include <util/generic/string.h>
#include <util/stream/file.h>

namespace NHnsw {

void WriteIndex(NYT::IClientPtr client, const TString& indexDataTable, size_t shardId, IOutputStream& out) {
    ui32 numItems = client->Get(indexDataTable + "/@num_items").AsUint64();
    out.Write(&numItems, sizeof(numItems));

    ui32 maxNeighbors = client->Get(indexDataTable + "/@max_neighbors").AsUint64();
    out.Write(&maxNeighbors, sizeof(maxNeighbors));

    ui32 levelSizeDecay = client->Get(indexDataTable + "/@level_size_decay").AsUint64();
    out.Write(&levelSizeDecay, sizeof(levelSizeDecay));

    auto reader = client->CreateTableReader<NYT::TNode>(
        NYT::TRichYPath(indexDataTable).AddRange(NYT::TReadRange().Exact(NYT::TReadLimit()
            .Key(shardId))));

    size_t levelSize = 0;
    size_t expectedLevelSize = numItems;
    size_t prevLevelId = 0;
    size_t prevNumNeighbors = Max<size_t>();

    auto checkLevel = [&](size_t levelId) {
        if (levelId == 0) {
            Y_VERIFY(levelSize <= expectedLevelSize);
            Y_VERIFY(prevNumNeighbors == maxNeighbors);
            const size_t numEmpty = expectedLevelSize - levelSize;
            TString zeros(numEmpty * maxNeighbors * sizeof(ui32), '\0');
            out.Write(zeros.data(), zeros.size());
        } else {
            Y_VERIFY(levelSize == expectedLevelSize);
            Y_VERIFY(prevNumNeighbors == Min<size_t>(maxNeighbors, levelSize - 1));
        }
        levelSize = 0;
        expectedLevelSize /= levelSizeDecay;
        prevNumNeighbors = Max<size_t>();
    };

    for (; reader->IsValid(); reader->Next()) {
        const auto& row = reader->GetRow();
        size_t levelId = row["level_id"].AsUint64();
        if (levelId != prevLevelId) {
            checkLevel(prevLevelId);
        }
        TString idsStr = row["neighbors"].AsString();
        out.Write(idsStr.data(), idsStr.size());
        prevLevelId = levelId;
        ++levelSize;
        Y_VERIFY(prevNumNeighbors == Max<size_t>() || prevNumNeighbors == idsStr.size() / sizeof(ui32));
        prevNumNeighbors = idsStr.size() / sizeof(ui32);
    }
    checkLevel(prevLevelId);
}

void WriteIndex(NYT::IClientPtr client, const TString& indexDataTable, size_t shardId, const TString& outputFilename) {
    TFixedBufferFileOutput out(outputFilename);
    WriteIndex(client, indexDataTable, shardId, out);
}

}

