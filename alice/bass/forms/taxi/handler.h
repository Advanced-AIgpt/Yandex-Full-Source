#pragma once

#include "statuses.h"

#include <alice/bass/forms/common/personal_data.h>
#include <alice/bass/forms/vins.h>

namespace NBASS {
namespace NTaxi {
class TTaxiApi;

class THandler : public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
    static void AddTaxiSuggest(TContext& context, TMaybe<TStringBuf> fromSlotName, TMaybe<TStringBuf> toSlotName);
    static NSc::TValue GetFormUpdate(const NSc::TValue& locationTo);

private:
    TResultValue CallToDriver(TContext& ctx, NTaxi::TTaxiApi& taxiApi);
    TResultValue CallToSupport(TContext& ctx, NTaxi::TTaxiApi& taxiApi);
    TResultValue CancelOrder(TContext& ctx, NTaxi::TTaxiApi& taxiApi);
    TResultValue ConfirmOrder(TContext& ctx, NTaxi::TTaxiApi& taxiApi, TPersonalDataHelper& personalData, const TPersonalDataHelper::TUserInfo& blackBoxInfo);
    TResultValue ChangePaymentOrTariff(TContext& ctx, NTaxi::TTaxiApi& taxiApi);
    TResultValue ChangeCard(TContext& ctx, NTaxi::TTaxiApi& taxiApi);
    TResultValue GenLinkToSite(TContext& ctx);
    TResultValue GetCancelOffer(TContext& ctx, NTaxi::TTaxiApi& taxiApi);
    TResultValue GetOrderStatus(TContext& ctx, NTaxi::TTaxiApi& taxiApi, TPersonalDataHelper& personalData, const TPersonalDataHelper::TUserInfo& blackBoxInfo);
    TResultValue MakeOffer(TContext& ctx, NTaxi::TTaxiApi& taxiApi, TPersonalDataHelper& personalData, const TPersonalDataHelper::TUserInfo& blackBoxInfo);
    TResultValue OpenApp(TContext& ctx);
    TResultValue SelectCard(TContext& ctx);
    TResultValue ShowLegal(TContext& ctx, NTaxi::TTaxiApi& taxiApi);
    TResultValue CancelOrderIfExists(TContext& ctx, TTaxiApi& taxiApi, TPersonalDataHelper& personalData, const TPersonalDataHelper::TUserInfo& blackBoxInfo);
    TResultValue FailToCancelOrder(TContext& ctx, EOrderStatus& status, NSc::TValue& orderData);
    TResultValue AskForCancelConfirmation(TContext& ctx, EOrderStatus& status, NSc::TValue& orderData);
};
} // namespace NTaxi
} // namespace NBASS
