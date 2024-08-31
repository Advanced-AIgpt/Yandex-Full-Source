#pragma once

#include <alice/tests/difftest/shooter/library/core/apps/app.h>
#include <alice/tests/difftest/shooter/library/core/requester/requester.h>

namespace NAlice::NShooter {

class IFactory : public TThrRefBase {
public:
    virtual ~IFactory() = default;
    virtual TIntrusivePtr<IApp> MakeApp() = 0;
    virtual THolder<IRequester> MakeRequester() = 0;
};

} // namespace NAlice::NShooter
