#pragma once

#include <alice/tests/difftest/shooter/library/core/apps/decorated_app.h>

namespace NAlice::NShooter {

class THollywoodApp : public TLocalDecoratedApp {
public:
    THollywoodApp(const IEngine& engine);
    THollywoodApp(const IEngine& engine, TIntrusivePtr<IApp> wrappee);

    TStringBuf Name() const override;

private:
    TStatus ExtraRun() override;
    TStatus ExtraPing() override;
};

} // namespace NAlice::NShooter
