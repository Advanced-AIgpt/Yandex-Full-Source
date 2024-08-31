#pragma once
#include <mapreduce/yt/interface/client.h>

namespace NMegamindLog {

void PrepareTimingTable(
    NYT::IClientPtr client,
    const NYT::TUserJobSpec& jobSpec,
    const NYT::TYPath& uniproxyLog,
    const NYT::TYPath& megamindAnalyticsLog,
    const NYT::TYPath& to
);

void PrepareUniproxyVinsTimings(
    NYT::IClientPtr client,
    const NYT::TYPath& from,
    const NYT::TYPath& to
);

} // namespace NMegamindLog

