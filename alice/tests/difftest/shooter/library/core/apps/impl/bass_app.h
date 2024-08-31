#pragma once

#include <alice/tests/difftest/shooter/library/core/apps/decorated_app.h>

namespace NAlice::NShooter {

class TBassApp : public TLocalDecoratedApp {
public:
    TBassApp(const IContext& ctx, const IEngine& engine, bool isHollywood);
    TBassApp(const IContext& ctx, const IEngine& engine, bool isHollywood, TIntrusivePtr<IApp> wrappee);

    TStringBuf Name() const override;

private:
    TStatus ExtraRun() override;
    TStatus ExtraPing() override;

private:
    const IContext& Ctx_;
    bool IsHollywood_;
};

} // namespace NAlice::NShooter
