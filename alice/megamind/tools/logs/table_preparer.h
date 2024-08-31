#pragma once
#include <mapreduce/yt/interface/client.h>

namespace NMegamindLog {

class TSimpleTablePreparer {
public:
    virtual ~TSimpleTablePreparer() = default;
    virtual void Prepare(NYT::IClientPtr client, const NYT::TYPath& megamindLog, const NYT::TYPath& to) const = 0;
};

} // namespace NMegamindLog
