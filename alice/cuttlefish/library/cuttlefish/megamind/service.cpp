#include "service.h"
#include "megamind.h"

namespace NAlice::NCuttlefish::NAppHostServices {

NThreading::TPromise<void> MegamindRun(const NAliceCuttlefishConfig::TConfig& cfg, NAppHost::TServiceContextPtr ctx, TLogContext logContext) {
    TMegamindServantPtr servant = TMegamindRunServant::Create(cfg, ctx, std::move(logContext));
    servant->OnNextInputSafe();
    return servant->Promise();
}

NThreading::TPromise<void> MegamindApply(const NAliceCuttlefishConfig::TConfig& cfg, NAppHost::TServiceContextPtr ctx, TLogContext logContext) {
    TMegamindServantPtr servant = TMegamindApplyServant::Create(cfg, ctx, std::move(logContext));
    servant->OnNextInputSafe();
    return servant->Promise();
}

}  // namespace NAlice::NCuttlefish::NAppHostServices
