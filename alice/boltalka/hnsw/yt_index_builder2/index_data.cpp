#include "index_data.h"

namespace NYtHnsw {

THnswYtIndexData LoadIndexDataFromTableAttribute(NYT::IClientPtr client,
                                                 const TString& tablePath,
                                                 const TString& attrName) {
    const auto indexDataNode = client->Get(tablePath + "/@" + attrName);
    THnswYtIndexData indexData;
    for (const auto& pair : indexDataNode["num_items"].AsMap()) {
        const ui64 shardId = FromString<ui64>(pair.first);
        const ui64 numItems = pair.second.AsUint64();
        indexData.ShardNumItems[shardId] = numItems;
    }

    indexData.MaxNeighbors= indexDataNode["max_neighbors"].AsUint64();
    indexData.LevelSizeDecay = indexDataNode["level_size_decay"].AsUint64();

    return indexData;
}

void SaveIndexDataToTableAttribute(NYT::IClientPtr client,
                                   const THnswYtIndexData& indexData,
                                   const TString& tablePath,
                                   const TString& attrName) {
    NYT::TNode indexDataNode;
    indexDataNode["max_neighbors"] = indexData.MaxNeighbors;
    indexDataNode["level_size_decay"] = indexData.LevelSizeDecay;
    indexDataNode["num_items"] = NYT::TNode();
    for (const auto& pair: indexData.ShardNumItems) {
        const ui64 shardId = pair.first;
        const ui64 numItems = pair.second;
        indexDataNode["num_items"][ToString(shardId)] = numItems;
    }

    client->Set(tablePath + "/@" + attrName, indexDataNode);
}

}
