#pragma once

#include <mapreduce/yt/interface/client.h>

namespace NAlice::NWonderlogs {

void MakeAsrPrepared(NYT::IClientPtr client, const TString& tmpDirectory, const TString& uniproxyPrepared,
                     const TVector<TString>& asrLogs, const TString& outputTable, const TString& errorTable,
                     const TInstant& timestampFrom, const TInstant& timestampTo, const TDuration& requestsShift);

} // namespace NAlice::NWonderlogs
