#pragma once

#include <alice/wonderlogs/protos/wonderlogs.pb.h>

#include <mapreduce/yt/interface/client.h>

namespace NAlice::NWonderlogs {

void MakeExpboxes(NYT::IClientPtr client, const TString& tmpDirectory, const TString& wonderlogs,
                  const TString& outputTable, const TString& errorTable);

} // namespace NAlice::NWonderlogs
