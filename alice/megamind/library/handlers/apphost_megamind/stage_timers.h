#pragma once

#include <alice/megamind/library/apphost_request/node.h>
#include <alice/megamind/library/requestctx/stage_timers.h>

namespace NAlice::NMegamind {

class TStageTimersAppHost : public TStageTimers {
public:
    explicit TStageTimersAppHost(TItemProxyAdapter& itemProxyAdapter)
        : ItemProxyAdapter_{itemProxyAdapter}
    {
    }

    void LoadFromContext(IAppHostCtx& ahCtx);

    void Upload(TStringBuf name, TInstant at) override;

private:
    TItemProxyAdapter& ItemProxyAdapter_;
};

} // namespace NAlice::NMegamind
