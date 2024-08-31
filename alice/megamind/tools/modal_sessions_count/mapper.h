#pragma once

#include "config_helpers.h"

#include <mapreduce/yt/interface/client.h>

#include <util/generic/vector.h>

namespace NAlice::NModalSessionMapper {

using TTableMapping = TVector<NYT::TYPath>;

void ComputeModalStats(TScenarioMaxTurns config, NYT::IClientPtr client, TVector<NYT::TYPath> from,
                       const NYT::TYPath& to);

} // namespace NAlice::NModalSessionMapper
