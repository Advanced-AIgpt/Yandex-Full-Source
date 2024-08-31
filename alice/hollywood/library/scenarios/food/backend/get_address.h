#pragma once

#include "auth_input.h"

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NFood {

namespace NApiGetAddress {
    // Retrieves delivery addresses of user
    // URL: https://core.eda.yandex.net/internal-api/v1/users/addresses

    void AddRequest(TScenarioHandleContext& ctx, const TLocation& location, const TAuthInput& authInput);

    class TRequestHandle : public TScenario::THandleBase {
    public:
        TString Name() const override;
        void Do(TScenarioHandleContext& ctx) const override;
    };

    enum class EError {
        SUCCESS,
        NO_RESPONSE,
        ADDRESS_NOT_FOUND,
        AUTHORIZATION_FAILED,
    };

    struct TResponseData {
        EError Error = EError::SUCCESS;
        NJson::TJsonValue Address;
    };

    TResponseData ReadResponse(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& req);

} // namespace NApiGetAddress 

} // namespace NAlice::NHollywood::NFood
