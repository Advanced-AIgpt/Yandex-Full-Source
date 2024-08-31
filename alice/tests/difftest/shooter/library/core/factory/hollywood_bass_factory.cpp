#include "hollywood_bass_factory.h"

#include <alice/tests/difftest/shooter/library/core/apps/impl/bass_app.h>
#include <alice/tests/difftest/shooter/library/core/apps/impl/joker_app.h>
#include <alice/tests/difftest/shooter/library/core/engine.h>

namespace NAlice::NShooter {

THollywoodBassFactory::THollywoodBassFactory(const IContext& ctx, const IEngine& engine)
    : Ctx_{ctx}
    , Engine_{engine}
{
}

TIntrusivePtr<IApp> THollywoodBassFactory::MakeApp() {
    TIntrusivePtr<IApp> app = nullptr;

    // set Joker
    if (Ctx_.Config().HasJokerConfig()) {
        app = MakeIntrusive<TJokerApp>(Ctx_);
    }

    // set Bass
    if (app == nullptr) {
        app = MakeIntrusive<TBassApp>(Ctx_, Engine_, /*isHollywood = */ true);
    } else {
        app = MakeIntrusive<TBassApp>(Ctx_, Engine_, /*isHollywood = */ true, app);
    }

    return app;
}

THolder<IRequester> THollywoodBassFactory::MakeRequester() {
    return MakeHolder<THollywoodBassRequester>(Ctx_, Engine_);
}

} // namespace NAlice::NShooter
