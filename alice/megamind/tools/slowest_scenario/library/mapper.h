#pragma once

#include <mapreduce/yt/interface/client.h>

#include <util/generic/vector.h>

namespace NAlice::NSlowestScenario {

using TTableMapping = TVector<NYT::TYPath>;

void ComputeTimings(const TString& clientRegexp, bool shouldSplitVinsIntents, NYT::IClientPtr client,
                    const TVector<NYT::TYPath>& from, const NYT::TYPath& to);

void ComputeTimingsOverWonderLogs(const TString& clientRegexp, bool shouldSplitVinsIntents, NYT::IClientPtr client,
                                  const TVector<NYT::TYPath>& from, const NYT::TYPath& to);

} // namespace NAlice::NSlowestScenario
