#include "netstat.h"

#include <util/stream/file.h>
#include <util/string/cast.h>


void TNetstatMetricsUpdater::UpdateMetrics(TMetrics& metrics) const {
    const TString s = TFileInput("/proc/net/sockstat").ReadAll();

    TStringBuf buf(s);

    buf = buf.After(' ').After(' ');
    metrics.SetSocketsTotal(FromStringWithDefault<uint64_t>(buf.Before('\n'), 0));

    buf = buf.After('\n').After(' ').After(' ');
    metrics.SetTcpSocketsInUse(FromStringWithDefault<uint64_t>(buf.Before(' '), 0));

    buf = buf.After(' ').After(' ');
    metrics.SetTcpSocketsOrphan(FromStringWithDefault<uint64_t>(buf.Before(' '), 0));

    buf = buf.After(' ').After(' ');
    metrics.SetTcpSocketsTimeWait(FromStringWithDefault<uint64_t>(buf.Before(' '), 0));

    buf = buf.After(' ').After(' ');
    metrics.SetTcpSocketsAllocated(FromStringWithDefault<uint64_t>(buf.Before(' '), 0));

    buf = buf.After(' ').After(' ');
    metrics.SetTcpSocketsKernelPages(FromStringWithDefault<uint64_t>(buf.Before('\n'), 0));
}
