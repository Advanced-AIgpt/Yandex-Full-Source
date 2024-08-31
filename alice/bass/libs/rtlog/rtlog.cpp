#include "rtlog.h"

namespace NBASS {
    NRTLog::TClient ConstructRTLogClient(const NBASSConfig::TRTLogConfigConst<TSchemeTraits>& config) {
        NRTLog::TClientOptions options{
            config.Async(),
            config.FlushPeriod(),
            config.FileStatCheckPeriod()
        };
        return NRTLog::TClient(TString{*config.FileName()}, TString{*config.ServiceName()}, options);
    }
}
