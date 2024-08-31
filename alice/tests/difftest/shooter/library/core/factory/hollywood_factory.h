#pragma once

#include <alice/tests/difftest/shooter/library/core/apps/app.h>
#include <alice/tests/difftest/shooter/library/core/factory/factory.h>
#include <alice/tests/difftest/shooter/library/core/requester/hollywood_requester.h>

namespace NAlice::NShooter {

class THollywoodFactory : public IFactory {
public:
    THollywoodFactory(const IEngine& engine);

    TIntrusivePtr<IApp> MakeApp() override;
    THolder<IRequester> MakeRequester() override;

private:
    const IEngine& Engine_;
};

} // namespace NAlice::NShooter
