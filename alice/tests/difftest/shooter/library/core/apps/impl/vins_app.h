#pragma once

#include <alice/tests/difftest/shooter/library/core/apps/decorated_app.h>

namespace NAlice::NShooter {

class TVinsApp : public TLocalDecoratedApp {
public:
    TVinsApp(const IContext& ctx, const IEngine& engine);
    TVinsApp(const IContext& ctx, const IEngine& engine, TIntrusivePtr<IApp> wrappee);

    TStringBuf Name() const override;

private:
    TStatus ExtraRun() override;
    TStatus ExtraPing() override;

private:
    const IContext& Ctx_;
};

} // namespace NAlice::NShooter
