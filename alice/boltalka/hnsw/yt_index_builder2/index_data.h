#pragma once

#include <mapreduce/yt/interface/client.h>

#include <util/generic/fwd.h>
#include <util/generic/hash.h>
#include <util/string/cast.h>
#include <util/ysaveload.h>

namespace NYtHnsw {

namespace {

static constexpr auto DefaultIndexDataAttrName = "index_data";

}

struct THnswYtIndexData {
    THashMap<ui64, ui64> ShardNumItems;
    ui32 MaxNeighbors = 0;
    ui32 LevelSizeDecay = 0;

    Y_SAVELOAD_DEFINE(ShardNumItems, MaxNeighbors, LevelSizeDecay);
};

THnswYtIndexData LoadIndexDataFromTableAttribute(NYT::IClientPtr client,
                                                 const TString& tablePath,
                                                 const TString& attrName = DefaultIndexDataAttrName);

void SaveIndexDataToTableAttribute(NYT::IClientPtr client,
                                   const THnswYtIndexData& indexData,
                                   const TString& tablePath,
                                   const TString& attrName = DefaultIndexDataAttrName);

}
