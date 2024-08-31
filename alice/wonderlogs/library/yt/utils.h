#pragma once

#include <mapreduce/yt/interface/client.h>

namespace NAlice::NWonderlogs {

void CreateTable(NYT::IClientBasePtr client, const TString& path, const TMaybe<TInstant>& expirationDate);

TString CreateRandomTable(NYT::IClientBasePtr client, TStringBuf directory, TStringBuf prefix, bool tempTable = true,
                          TDuration ttl = TDuration::Days(7));

struct TMergeAttributes {
    TMergeAttributes(double compressionRatio, const TMaybe<ui64>& dataWeight, const TMaybe<ui64>& compressedDataSize,
                     const TString& erasureCodec);
    ui64 DataSizePerJob = 0;
    ui64 DesiredChunkSize = 0;
};

} // namespace NAlice::NWonderlogs