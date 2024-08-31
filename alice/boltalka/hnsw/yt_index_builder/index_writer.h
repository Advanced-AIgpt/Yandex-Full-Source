#pragma once
#include <mapreduce/yt/interface/client.h>
#include <util/generic/fwd.h>

class IOutputStream;

namespace NHnsw {

void WriteIndex(NYT::IClientPtr client, const TString& indexDataTable, size_t shardId, IOutputStream& out);
void WriteIndex(NYT::IClientPtr client, const TString& indexDataTable, size_t shardId, const TString& outputFilename);

}

