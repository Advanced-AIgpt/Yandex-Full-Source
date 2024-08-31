#pragma once

#include <alice/hollywood/library/scenarios/onboarding/proto/onboarding.pb.h>

#include <alice/hollywood/library/framework/framework.h>

namespace NAlice::NHollywoodFw::NOnboarding {

    class TOnboardingScenario: public TScenario {
    public:
        TOnboardingScenario();

        TRetScene Dispatch(const TRunRequest&,
                           const TStorage&,
                           const TSource&) const;
    };

}
