#include "fs.h"

#include <util/stream/file.h>
#include <util/string/cast.h>


void TFileHandlerMetricsUpdater::UpdateMetrics(TMetrics& metrics) const {
    const TString s = TFileInput("/proc/sys/fs/file-nr").ReadAll();

    TStringBuf buf(s);

    metrics.SetFileHandlersAllocated(FromStringWithDefault<float>(buf.Before('\t'), 0));

    buf = buf.After('\t');
    metrics.SetFileHandlersUnused(FromStringWithDefault<float>(buf.Before('\t'), 0));

    buf = buf.After('\t');
    metrics.SetFileHandlersMax(FromStringWithDefault<float>(buf.Before('\n'), 0));
}
