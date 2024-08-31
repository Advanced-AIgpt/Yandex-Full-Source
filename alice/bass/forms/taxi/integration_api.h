#pragma once

#include "statuses.h"

#include <alice/bass/forms/common/personal_data.h>
#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/vins.h>

#include <alice/bass/libs/fetcher/request.h>
#include <alice/bass/libs/fetcher/neh.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/map.h>
#include <util/generic/string.h>

namespace NBASS {
namespace NTaxi {
inline const THashSet<TStringBuf> BANNED_TARIFFS{TStringBuf("child_tariff"), TStringBuf("cargo")};

inline constexpr TStringBuf PAYMENT_CASH = "cash";
inline constexpr TStringBuf PAYMENT_CARD = "card";
inline constexpr TStringBuf PAYMENT_CORP = "corp";
inline const THashSet<TStringBuf> SUPPORTED_PAYMENT_METHODS{PAYMENT_CASH, PAYMENT_CARD, PAYMENT_CORP};

struct TResponse {
    NHttpFetcher::TResponse::THttpCode HttpCode = 0;
    NSc::TValue Data;
    TTaxiApiError taxiApiError;
};

class TTaxiApi {
public:
    TTaxiApi(TContext& ctx, const TPersonalDataHelper::TUserInfo& userInfo, TStringBuf offerSlot, TStringBuf checkedPaymentSlot, TStringBuf checkedTariffSlot, TStringBuf cardNumberSlot);
    explicit TTaxiApi(TContext& ctx);
    bool SetPaymentMethodUnsafe(TStringBuf type, TStringBuf id);
    TCancelResult CancelOrder(TStringBuf orderId);
    TConfigureUserIdResult ConfigureUserId(NSc::TValue& userProfile);
    TGetLocationInfoResult GetLocationInfo(const NSc::TValue& location);
    TGetPaymentMethodsResult GetPaymentMethods(NSc::TValue& methods, const NSc::TValue& location, TStringBuf type = {});
    TGetPriceResult GetPrice(const NSc::TValue& locationFrom, const NSc::TValue& locationTo, NSc::TValue* resultData);
    TGetStatusResult GetStatus(EOrderStatus* resultStatus, NSc::TValue* resultData);
    TMakeOrderResult MakeOrder(const NSc::TValue& locationFrom, const NSc::TValue& locationTo, TStringBuf comment = {}, TString* orderId = nullptr);
    TSendMessageToSupportResult SendMessageToSupport(TStringBuf message = {});
    TSetPaymentMethodResult SetPaymentMethod(TStringBuf method, const NSc::TValue& location, NSc::TValue& history);
    TSetTariffResult SetTariff(TStringBuf tariff);
    void GetTariffsList(NSc::TValue& tariffs);
    void SendPushes(const TString& event, NSc::TValue serviceData = {});

private:
    bool Request(const NSc::TValue& requestData, TResponse& responseData, TStringBuf path, bool isSupport = false);
    NSc::TValue LocationToCoordsList(const NSc::TValue& location);
    NSc::TValue LocationToTaxiFormat(const NSc::TValue& location);
    void SetPaymentMethodImpl(NSc::TValue& slot, TStringBuf type, TStringBuf id);
    void AddUserInfo(NSc::TValue& request);
    void AddAliceParameters(NSc::TValue& request) const;

private:
    TSourcesRequestFactory ApiFactory;
    TString Name;
    TString Phone;
    TString TaxiUid;
    TString YandexUid;
    TString Offer;
    TString Uuid;
    TString DeviceId;
    TString ClientId;
    TString LoginId;
    TContext::TSlot* PaymentMethod;
    TContext::TSlot* CardNumber;
    TContext::TSlot* Tariff;
    IGlobalContext& GlobalCtx;
    THashMap<TString, TString> AvailableTariffs;
    TVector<TString> AvailableTariffsList;

    bool IsSmartDevice = false;
    bool FullInit;
    bool HasLocationInfo = false;
};
}  // namespace NTaxi
}  // namespace NBASS
