#pragma once

#include <alice/tests/difftest/shooter/library/core/apps/decorated_app.h>

namespace NAlice::NShooter {

class TRedisApp : public TLocalDecoratedApp {
public:
    TRedisApp(const IEngine& engine);
    TRedisApp(const IEngine& engine, TIntrusivePtr<IApp> wrappee);

    TStringBuf Name() const override;

private:
    TStatus ExtraRun() override;
    TStatus ExtraPing() override;
};

} // namespace NAlice::NShooter
