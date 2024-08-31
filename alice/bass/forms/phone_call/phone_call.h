#pragma once

#include <alice/bass/forms/vins.h>

namespace NBASS {

class TPhoneCallHandler : public IHandler {
public :
    TResultValue Do(TRequestHandler& r) override;
    static void Register(THandlersMap* handlers);

    static const THashMap<NGeobase::TId, NSc::TValue>& GetEmergencyPhoneBook();

private:
    struct TPhoneBook {
        TPhoneBook();
        THashMap<NGeobase::TId, NSc::TValue> EmergencyPhoneBook;
    };

    NSc::TValue GetRegionalEmergencyServices(NGeobase::TId userCountry);
    NSc::TValue GetEmergencyServiceInfo(NGeobase::TId userCountry, TStringBuf name);
    void AddEmergencyServiceSuggests(NGeobase::TId userCountry, TStringBuf serviceId, TContext& ctx);

    TResultValue DoImpl(TRequestHandler& r);
    TResultValue HandleEmergencyCall(TRequestHandler& r, NSc::TValue* recipientInfo);

    void FillCallData(const NSc::TValue& recipientInfo, TContext& ctx, bool useDivCard = true) const;

private:
    static const THashSet<TStringBuf> EmergencyServices;
};

}
