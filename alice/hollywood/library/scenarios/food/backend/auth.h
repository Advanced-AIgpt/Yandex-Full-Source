#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NFood {

namespace NApiGetTaxiUid {
    // Find nearest restaurant.

    void AddRequest(TScenarioHandleContext& ctx, const TString& phone, const TString& yandexUid);

    class TRequestHandle : public TScenario::THandleBase {
    public:
        TString Name() const override;
        void Do(TScenarioHandleContext& ctx) const override;
    };

    enum class EError {
        SUCCESS,
        NO_RESPONSE,
        FAILED_TO_AUTHENTICATE
    };

    struct TResponseData {
        EError Error = EError::SUCCESS;
        TString TaxiUid;
    };

    TResponseData ReadResponse(TScenarioHandleContext& ctx);

    TString GetTaxiUid(TScenarioHandleContext& ctx);

} // namespace NApiGetTaxiUid

} // namespace NAlice::NHollywood::NFood
