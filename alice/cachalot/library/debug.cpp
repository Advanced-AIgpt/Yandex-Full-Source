#include <alice/cachalot/library/debug.h>

namespace NCachalot {

    TChroniclerPtr MakeLoggerWithRtLogTokenFromAppHostContext(const NAppHost::IServiceContext& ahContext) {
        if (const TMaybe<TString> rtLogToken = NAlice::NCuttlefish::TryGetRtLogTokenFromAppHostContext(ahContext)) {
            return MakeIntrusive<TChronicler>(rtLogToken.GetRef());
        }
        return MakeIntrusive<TChronicler>();
    }

}   // namespace NCachalot
