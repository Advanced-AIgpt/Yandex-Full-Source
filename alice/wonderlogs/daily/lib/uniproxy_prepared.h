#pragma once

#include <mapreduce/yt/interface/client.h>

namespace NAlice::NWonderlogs {

void MakeUniproxyPrepared(NYT::IClientPtr client, const TString& tmpDirectory, const TVector<TString>& uniproxyEvents,
                          const TString& outputTable, const TString& errorTable, const TInstant& timestampFrom,
                          const TInstant& timestampTo, const TDuration& requestsShift);

} // namespace NAlice::NWonderlogs
