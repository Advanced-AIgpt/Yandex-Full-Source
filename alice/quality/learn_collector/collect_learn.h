#pragma once

#include <mapreduce/yt/interface/fwd.h>

#include <util/generic/fwd.h>
#include <util/system/types.h>

class TDate;

namespace NAlice {
    enum class EPlatformType {
        Quasar,
        General,
    };
    void CollectDates(const TDate& startDate, const TDate& endDate, const NYT::TYPath& outputPath,
        const NYT::TYPath& tmpDir, NYT::IClientPtr& client, const EPlatformType platform, const ui32 maxContextDepth,
        const ui32 parallelOperations = 0);
}
