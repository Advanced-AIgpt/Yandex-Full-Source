#pragma once

#include <mapreduce/yt/interface/client.h>

namespace NAlice::NWonderlogs {

void ExtractYsonDataFromWonderlogs(NYT::IClientPtr client, const TVector<TString>& wonderlogs,
                                   const TString& outputTable, const TString& errorTable);

} // namespace NAlice::NWonderlogs
