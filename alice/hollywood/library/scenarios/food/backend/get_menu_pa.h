#pragma once

#include "auth_input.h"

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NFood {

namespace NApiFindPlacePA {
    // Find nearest restaurant.

    void AddRequest(TScenarioHandleContext& ctx, const TLocation& location, const TAuthInput& authInput);

    class TRequestHandle : public TScenario::THandleBase {
    public:
        TString Name() const override;
        void Do(TScenarioHandleContext& ctx) const override;
    };

    enum class EError {
        SUCCESS,
        NO_RESPONSE,
        PLACE_NOT_FOUND,
        AUTHORIZATION_FAILED,
    };

    struct TResponseData {
        EError Error = EError::SUCCESS;
        TString PlaceSlug;
        TString TaxiUid;
    };

    TResponseData ReadResponse(TScenarioHandleContext& ctx);

} // namespace NApiFindPlacePA

namespace NApiGetMenuPA {
    // Get menu of nearest restaurant.

    void AddRequest(TScenarioHandleContext& ctx, const TLocation& location, const TAuthInput& authInput, const TString& placeSlug);

    class TRequestHandle : public TScenario::THandleBase {
    public:
        TString Name() const override;
        void Do(TScenarioHandleContext& ctx) const override;
    };

    enum class EError {
        SUCCESS,
        NO_RESPONSE,
        PLACE_NOT_FOUND,
        AUTHORIZATION_FAILED,
    };

    struct TResponseData {
        EError Error = EError::SUCCESS;
        NJson::TJsonValue Menu;
        TString TaxiUid;
        TString PlaceSlug;
    };

    TResponseData ReadResponse(TScenarioHandleContext& ctx);

} // namespace NApiGetMenuPA

} // namespace NAlice::NHollywood::NFood
