#pragma once

#include <alice/tests/difftest/shooter/library/core/apps/app.h>
#include <alice/tests/difftest/shooter/library/core/factory/factory.h>
#include <alice/tests/difftest/shooter/library/core/requester/megamind_requester.h>

namespace NAlice::NShooter {

class TMegamindFactory : public IFactory {
public:
    TMegamindFactory(const IContext& ctx, const IEngine& engine);
    ~TMegamindFactory();

    TIntrusivePtr<IApp> MakeApp() override;
    THolder<IRequester> MakeRequester() override;

private:
    const IContext& Ctx_;
    const IEngine& Engine_;
};

} // namespace NAlice::NShooter
