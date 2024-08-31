#pragma once

#include <alice/wonderlogs/protos/megamind_prepared.pb.h>
#include <alice/wonderlogs/protos/uniproxy_prepared.pb.h>

#include <mapreduce/yt/interface/client.h>

namespace NAlice::NWonderlogs {

void MakeMegamindPrepared(NYT::IClientPtr client, const TString& tmpDirectory, const TString& uniproxyPrepared,
                          const TVector<TString>& megamindAnalyticsLogs, const TString& outputTable,
                          const TString& errorTable, const TInstant& timestampFrom, const TInstant& timestampTo);

} // namespace NAlice::NWonderlogs
