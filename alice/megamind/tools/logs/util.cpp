#include "util.h"

namespace NMegamindLog {

bool IsRealReqId(const TString& reqId) {
    return !reqId.Empty() &&
        reqId != TStringBuf("d4fa807b-b5cc-49d7-8a82-8b037dfedff8") &&
        !reqId.StartsWith(TStringBuf("ffffffff-ffff-ffff")) &&
        !reqId.StartsWith(TStringBuf("dddddddd-dddd-dddd"));
}

} // namespace NMegamindLog
