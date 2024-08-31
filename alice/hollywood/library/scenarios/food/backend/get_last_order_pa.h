#pragma once

#include "auth_input.h"

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NFood {

namespace NApiGetLastOrderPA {
    // Request last order of user.
    // URL: https://eda.yandex.ru/api/v1/orders

    void AddRequest(TScenarioHandleContext& ctx, const TAuthInput& authInput);

    class TRequestHandle : public TScenario::THandleBase {
    public:
        TString Name() const override;
        void Do(TScenarioHandleContext& ctx) const override;
    };

    enum class EError {
        SUCCESS,
        NO_RESPONSE,
        NO_ORDER,
        AUTHORIZATION_FAILED,
    };

    struct TResponseData {
        EError Error = EError::SUCCESS;
        NJson::TJsonValue LastOrder;
        TString TaxiUid;
    };

    TResponseData ReadResponse(TScenarioHandleContext& ctx);

} // namespace NApiGetLastOrderPA

} // namespace NAlice::NHollywood::NFood
