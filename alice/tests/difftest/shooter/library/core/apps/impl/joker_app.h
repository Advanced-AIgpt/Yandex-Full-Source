#pragma once

#include <alice/tests/difftest/shooter/library/core/apps/decorated_app.h>

namespace NAlice::NShooter {

class TJokerApp : public IApp {
public:
    TJokerApp(const IContext& ctx);

    TStringBuf Name() const override;
    void Init() override;
    TStatus Run() override;
    TStatus Ping() override;
    void Stop() override;

private:
    const IContext& Ctx_;
};

} // namespace NAlice::NShooter
