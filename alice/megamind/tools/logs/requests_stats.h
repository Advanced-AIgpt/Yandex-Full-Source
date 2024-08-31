#pragma once

#include <mapreduce/yt/interface/client.h>

#include "table_preparer.h"

namespace NMegamindLog {

class TRequestsStatsTablePreparer : public TSimpleTablePreparer {
    void Prepare(NYT::IClientPtr client, const NYT::TYPath& megamindLog, const NYT::TYPath& to) const override;
};

} // namespace NMegamindLog
