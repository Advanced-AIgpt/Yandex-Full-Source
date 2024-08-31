#include "hollywood_factory.h"

#include <alice/tests/difftest/shooter/library/core/apps/impl/hollywood_app.h>
#include <alice/tests/difftest/shooter/library/core/engine.h>

namespace NAlice::NShooter {

THollywoodFactory::THollywoodFactory(const IEngine& engine)
    : Engine_{engine}
{
}

TIntrusivePtr<IApp> THollywoodFactory::MakeApp() {
    return MakeIntrusive<THollywoodApp>(Engine_);
}

THolder<IRequester> THollywoodFactory::MakeRequester() {
    return MakeHolder<THollywoodRequester>(Engine_);
}

} // namespace NAlice::NShooter
