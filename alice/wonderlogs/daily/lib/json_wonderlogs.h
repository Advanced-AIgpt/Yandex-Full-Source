#pragma once

#include <mapreduce/yt/interface/client.h>

namespace NAlice::NWonderlogs {

void MakeJsonWonderlogs(NYT::IClientPtr client, const TString& tmpDirectory, const TString& wonderlogs,
                        const TString& outputTable, const TString& errorTable);

} // namespace NAlice::NWonderlogs
