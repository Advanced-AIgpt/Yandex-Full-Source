#pragma once

#include <alice/tests/difftest/shooter/library/core/apps/decorated_app.h>

namespace NAlice::NShooter {

class TMegamindApp : public TLocalDecoratedApp {
public:
    TMegamindApp(const IContext& ctx, const IEngine& engine);
    TMegamindApp(const IContext& ctx, const IEngine& engine, TIntrusivePtr<IApp> wrappee);

    TStringBuf Name() const override;

private:
    TStatus ExtraRun() override;
    TStatus ExtraPing() override;

private:
    const IContext& Ctx_;
};

} // namespace NAlice::NShooter
