#pragma once

#include <mapreduce/yt/interface/client.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NMatrix::NNotificator::NAnalytics {

void ReducePushEventsById(
    NYT::IClientPtr client,
    const TVector<TString>& tables,
    const TString& outputTable
);

} // namespace NMatrix::NNotificator::NAnalytics
