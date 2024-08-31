#pragma once
#include "index_data.h"

#include <mapreduce/yt/interface/client.h>
#include <util/generic/fwd.h>

class IOutputStream;

namespace NYtHnsw {

void WriteAllIndexes(NYT::IClientPtr client, const TString& inputTable, const TString& outputFilename, const TString& offsetsFilename);
void WriteAllIndexes(NYT::IClientPtr client, const TString& inputTable, IOutputStream& out, TVector<ui64>& offsets);
void WriteIndex(NYT::IClientPtr client, const TString& indexDataTable, size_t shardId, IOutputStream& out);
void WriteIndex(NYT::IClientPtr client, const TString& indexDataTable, size_t shardId, const TString& outputFilename);
void WriteIndex(const THnswYtIndexData& indexData, NYT::TTableReader<NYT::TNode>* input, size_t shardId, IOutputStream& out);
void WriteIndex(const THnswYtIndexData& indexData, NYT::TTableReader<NYT::TNode>* input, size_t shardId, IOutputStream& out);

}

