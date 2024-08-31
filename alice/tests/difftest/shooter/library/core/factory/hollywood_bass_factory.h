#pragma once

#include <alice/tests/difftest/shooter/library/core/apps/app.h>
#include <alice/tests/difftest/shooter/library/core/factory/factory.h>
#include <alice/tests/difftest/shooter/library/core/requester/hollywood_bass_requester.h>

namespace NAlice::NShooter {

class THollywoodBassFactory : public IFactory {
public:
    THollywoodBassFactory(const IContext& ctx, const IEngine& engine);

    TIntrusivePtr<IApp> MakeApp() override;
    THolder<IRequester> MakeRequester() override;

private:
    const IContext& Ctx_;
    const IEngine& Engine_;
};

} // namespace NAlice::NShooter
