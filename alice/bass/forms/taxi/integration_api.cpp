#include "integration_api.h"

#include <alice/bass/forms/geo_resolver.h>

#include <alice/bass/libs/client/experimental_flags.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/push_notification/create_callback_data.h>
#include <alice/bass/libs/push_notification/handlers/taxi_push.h>
#include <alice/bass/libs/push_notification/request.h>

#include <alice/bass/util/error.h>

#include <util/string/cast.h>

namespace NBASS {
namespace NTaxi {
namespace {
constexpr TStringBuf PAYMENTSMETHODS = "paymentmethods";
constexpr TStringBuf PROFILE = "profile";
constexpr TStringBuf ZONEINFO = "zoneinfo";
constexpr TStringBuf CANCEL = "orders/cancel";
constexpr TStringBuf COMMIT = "orders/commit";
constexpr TStringBuf DRAFT = "orders/draft";
constexpr TStringBuf ESTIMATE = "orders/estimate";
constexpr TStringBuf SEARCH = "orders/search";
constexpr TStringBuf SUPPORT = "webhook/alice";
constexpr TStringBuf ALICE_SOURCE_ID = "alice";

constexpr TStringBuf STRING_TYPE = "string";

constexpr TStringBuf DEFAULT_TARIFF_NAME = "Эконом";
constexpr TStringBuf DEFAULT_TARIFF_TYPE = "econom";

constexpr TStringBuf DEFAULT_SUPPORT_MESSAGE = "Привет! Это тикет из Алисы, нужно позвонить пользователю и узнать, что пошло не так =)";

TStringBuf CutCardNumber(TStringBuf cardNumber) {
    size_t cardTailSize = 4;
    if (cardNumber.Size() > cardTailSize) {
        cardNumber.RSeek(cardTailSize);
    }
    return cardNumber;
}
} // namespace

TTaxiApi::TTaxiApi(TContext& ctx, const TPersonalDataHelper::TUserInfo& userInfo, TStringBuf offerSlot, TStringBuf checkedPaymentSlot, TStringBuf checkedTariffSlot, TStringBuf cardNumberSlot)
    : ApiFactory{ctx.GetSources()}
    , Name{userInfo.GetFirstName() ? userInfo.GetFirstName() : TStringBuf("Аноним")}
    , Phone{userInfo.GetPhone()}
    , YandexUid{userInfo.GetUid()}
    , Offer{ctx.GetOrCreateSlot(offerSlot, STRING_TYPE)->Value.GetString()}
    , Uuid{ctx.Meta().UUID()}
    , DeviceId{ctx.GetDeviceId()}
    , ClientId{ctx.Meta().ClientId()}
    , LoginId{userInfo.GetLoginId()}
    , PaymentMethod{ctx.GetOrCreateSlot(checkedPaymentSlot, STRING_TYPE)}
    , CardNumber{ctx.GetOrCreateSlot(cardNumberSlot, STRING_TYPE)}
    , Tariff{ctx.GetOrCreateSlot(checkedTariffSlot, STRING_TYPE)}
    , GlobalCtx{ctx.GlobalCtx()}
    , IsSmartDevice{ctx.MetaClientInfo().IsSmartSpeaker() || ctx.MetaClientInfo().IsTvDevice()}
    , FullInit{true}
{
    if (ctx.HasExpFlag(EXPERIMENTAL_FLAG_TAXI_CARD_CORP)) {
        Phone = TStringBuf("+79161324447");
        YandexUid = TStringBuf("4021923944");
    }
}

TTaxiApi::TTaxiApi(TContext& ctx)
    : ApiFactory{ctx.GetSources()}
    , GlobalCtx{ctx.GlobalCtx()}
    , FullInit{false}
{}

bool TTaxiApi::Request(const NSc::TValue& requestData, TResponse& responseData, TStringBuf path, bool isSupport) {
    NHttpFetcher::TRequestPtr request = ApiFactory.TaxiApi(path, isSupport).Request();
    request->SetBody(requestData.ToJson());
    request->SetMethod("POST");
    request->AddHeader("Content-Type", "application/json");

    LOG(DEBUG) << "Taxi request: " << requestData.ToJson() << Endl;
    NHttpFetcher::TResponse::TRef response = request->Fetch()->Wait();

    responseData.HttpCode = response->Code;
    if (response->IsError()) {
        LOG(ERR) << "Taxi API fetch error: " << response->GetErrorText() << response->Data << Endl;
        TStringBuf errorData = response->Data;
        if (errorData.Contains("class") && errorData.Contains("not found")) {
            responseData.taxiApiError = ETaxiApiError::TariffNotFound;
        } else if (errorData.Contains("Invalid phone number")) {
            responseData.taxiApiError = ETaxiApiError::InvalidPhoneNumber;
        }
        return false;
    } else if (response->Code == HttpCodes::HTTP_OK || response->Code == HttpCodes::HTTP_ACCEPTED) {
        responseData.Data = NSc::TValue::FromJson(response->Data);
        LOG(DEBUG) << "Taxi response: " << responseData.Data.ToJson() << Endl;
        return true;
    } else {
        LOG(ERR) << "Taxi API server error: " << response->Data << Endl;
        return false;
    }
}

void TTaxiApi::SendPushes(const TString& event, NSc::TValue serviceData) {
    Y_ASSERT(FullInit);
    NPushNotification::TCallbackDataSchemeHolder callbackData = NPushNotification::GenerateCallbackDataSchemeHolder(Uuid, DeviceId, YandexUid, ClientId);
    NPushNotification::TResult requestsVariant = NPushNotification::GetRequestsLocal(GlobalCtx, serviceData, "taxi", event, callbackData);
    NPushNotification::SendPushUnsafe(requestsVariant, event);
}

NSc::TValue TTaxiApi::LocationToCoordsList(const NSc::TValue& location) {
    auto coords = NSc::TValue().SetArray();
    coords.Push(location["location"]["lon"]);
    coords.Push(location["location"]["lat"]);
    return coords;
}

NSc::TValue TTaxiApi::LocationToTaxiFormat(const NSc::TValue& location) {
    NSc::TValue result;
    const NSc::TValue& data = location.Has("geo") ? location["geo"] : location;
    result["country"] = data["country"];
    result["fullname"] = data["address_line"];
    result["short_text"] = data["address_line"];
    result["geopoint"] = LocationToCoordsList(location);
    result["locality"] = data["city"];
    result["porchnumber"] = ""; // TODO(ar7is7@): fill it
    result["premisenumber"] = data["house"];
    result["thoroughfare"] = data["street"];

    return result;
}

void TTaxiApi::AddUserInfo(NSc::TValue& request) {
    Y_ASSERT(FullInit);
    request["user"]["phone"] = Phone;
    request["user"]["yandex_uid"] = YandexUid;
    if (TaxiUid)
        request["user"]["user_id"] = TaxiUid;
}

TConfigureUserIdResult TTaxiApi::ConfigureUserId(NSc::TValue& userProfile) {
    Y_ASSERT(FullInit);
    TStringBuf uid = userProfile["id"].GetString();
    if (uid) {
        TaxiUid = uid;
        return TConfigureUserIdResult();
    }

    NSc::TValue request;
    TResponse response;
    AddUserInfo(request);
    request["sourceid"] = ALICE_SOURCE_ID;
    request["name"] = Name;

    if (Request(request, response, PROFILE)) {
        TaxiUid = response.Data["user_id"].GetString();
        userProfile["id"].SetString(TaxiUid);
        if (response.Data["has_debt"].GetBool()) {
            return EConfigureUserIdError::UserHasDebt;
        }

        // if taxi doesn't return this field, we suppose it to be true
        userProfile["is_phonish"].SetBool(response.Data["suggest_portal_signin"].GetBool(true));
        return TConfigureUserIdResult();
    }
    if (response.taxiApiError == ETaxiApiError::InvalidPhoneNumber) {
        return EConfigureUserIdError::InvalidPhoneNumber;
    }

    switch (response.HttpCode) {
        case HttpCodes::HTTP_FORBIDDEN:
            return EConfigureUserIdError::UserBlocked;
        default:
            return EConfigureUserIdError::NoValidResponse;
    }
}

TGetPriceResult TTaxiApi::GetPrice(const NSc::TValue& locationFrom, const NSc::TValue& locationTo, NSc::TValue* resultData) {
    Y_ASSERT(FullInit);
    NSc::TValue request;
    TResponse response;
    AddUserInfo(request);
    request["sourceid"] = ALICE_SOURCE_ID;
    TStringBuf tariff = Tariff->Value["type"].GetString();
    tariff = tariff ? tariff : DEFAULT_TARIFF_TYPE;
    request["selected_class"].SetString(tariff);

    if (!PaymentMethod->Value["type"].IsNull()) {
        request["payment"]["type"] = PaymentMethod->Value["type"];
        if (!PaymentMethod->Value["id"].IsNull()) {
            request["payment"]["payment_method_id"] = PaymentMethod->Value["id"];
        }
    }

    request["route"].SetArray();
    request["route"].Push(LocationToCoordsList(locationFrom));
    request["route"].Push(LocationToCoordsList(locationTo));

    if (Request(request, response, ESTIMATE)) {
        Offer = response.Data["offer"].GetString();
        (*resultData)["offer"] = Offer;
        (*resultData)["is_fixed_price"].SetBool(response.Data["is_fixed_price"].GetBool());
        NSc::TValue& tariffInfo = response.Data["service_levels"][0];
        (*resultData)["price"] = tariffInfo["price"].GetString();
        (*resultData)["price_raw"] = tariffInfo["price_raw"].GetString();
        (*resultData)["estimate_route_time_minutes"] = tariffInfo["time_raw"].ForceString();
        // just rounding, don't worry
        (*resultData)["waiting_time_minutes"] = ToString((tariffInfo["estimated_waiting"]["seconds"].ForceIntNumber() + 30) / 60);
        return TGetPriceResult();
    }
    switch (response.HttpCode) {
        case HttpCodes::HTTP_NOT_FOUND:
            return EGetPriceError::LocationIsNotSupported;
        default: // TODO(ar7is7): add handling for EGetPriceError::CantConstructRoute
            return EGetPriceError::NoValidResponse;
    }
}

void TTaxiApi::SetPaymentMethodImpl(NSc::TValue& slot, TStringBuf type, TStringBuf id) {
    slot["type"].SetString(type);
    slot["id"].SetString(id);
}

bool TTaxiApi::SetPaymentMethodUnsafe(TStringBuf type, TStringBuf id) {
    if (!type || !id) {
        return false;
    }
    SetPaymentMethodImpl(PaymentMethod->Value, type, id);
    return true;
}

void TTaxiApi::AddAliceParameters(NSc::TValue& request) const {
    NSc::TValue& params = request["alice_parameters"];
    params["origin_login_id"].SetString(LoginId);
    params["is_smart_device"].SetBool(IsSmartDevice);
}

TSetPaymentMethodResult TTaxiApi::SetPaymentMethod(TStringBuf method, const NSc::TValue& location, NSc::TValue& history) {
    Y_ASSERT(FullInit);
    NSc::TValue& lastPaymentMethod = history["last_payment_method"];
    NSc::TValue& lastCard = history["last_card"];

    TResponse response;
    NSc::TValue request;
    request["point"] = LocationToCoordsList(location);
    request["user_id"] = TaxiUid;
    request["sourceid"] = ALICE_SOURCE_ID;
    TStringBuf tariff = Tariff->Value["type"].GetString();
    tariff = tariff ? tariff : DEFAULT_TARIFF_TYPE;
    request["class"].SetString(tariff);
    AddAliceParameters(request);

    if (Request(request, response, PAYMENTSMETHODS)) {
        NSc::TValue& responseLastPayment = response.Data["last_payment_method"];
        if (lastPaymentMethod["type"].IsNull() && SUPPORTED_PAYMENT_METHODS.find(responseLastPayment["type"].GetString()) != SUPPORTED_PAYMENT_METHODS.end()) {
            lastPaymentMethod.CopyFrom(responseLastPayment);
        }

        TStringBuf lastId;
        if (method == PAYMENT_CARD && lastCard["id"].IsString()) {
            lastId = lastCard["id"].GetString();
        } else if (!method || method == lastPaymentMethod["type"].GetString()) {
            lastId = lastPaymentMethod["id"].GetString();
        }

        TStringBuf corpId, cardId, cashId, cardNum;
        for (const NSc::TValue& responseMethod : response.Data["methods"].GetArray()) {
            if (responseMethod["can_order"].GetBool() && responseMethod["zone_available"].GetBool()) {
                TStringBuf paymentType = responseMethod["type"].GetString();
                if (lastId && lastId == responseMethod["id"].GetString()) {
                    // we found last payment method and it's available
                    TStringBuf selectedType = responseMethod["type"].GetString();
                    TStringBuf selectedId = responseMethod["id"].GetString();
                    SetPaymentMethodUnsafe(paymentType, lastId);
                    SetPaymentMethodImpl(lastPaymentMethod, selectedType, selectedId);
                    if (paymentType == PAYMENT_CARD) {
                        SetPaymentMethodImpl(lastCard, selectedType, selectedId);
                        cardNum = CutCardNumber(responseMethod["description"].GetString());
                        CardNumber->Value.SetString(cardNum);
                    }
                    return TSetPaymentMethodResult();
                } else if (paymentType == PAYMENT_CORP) {
                    corpId = responseMethod["id"].GetString();
                } else if (paymentType == PAYMENT_CARD) {
                    cardId = responseMethod["id"].GetString();
                    cardNum = CutCardNumber(responseMethod["description"].GetString());
                } else if (paymentType == PAYMENT_CASH) {
                    cashId = responseMethod["id"].GetString();
                }
            }
        }

        TStringBuf selectedType, selectedId;
        if (corpId && (!method || method == PAYMENT_CORP)) {
            selectedId = corpId;
            selectedType = PAYMENT_CORP;
        } else if (cardId && (!method || method == PAYMENT_CARD)) {
            selectedId = cardId;
            selectedType = PAYMENT_CARD;
            CardNumber->Value.SetString(cardNum);
        } else if (cashId && (!method || method == PAYMENT_CASH)) {
            selectedId = cashId;
            selectedType = PAYMENT_CASH;
        }

        if (selectedId && selectedType) {
            SetPaymentMethodUnsafe(selectedType, selectedId);
            SetPaymentMethodImpl(lastPaymentMethod, selectedType, selectedId);
            if (selectedType == PAYMENT_CARD) {
                SetPaymentMethodImpl(lastCard, selectedType, selectedId);
            }
            return TSetPaymentMethodResult();
        }

        return ESetPaymentMethodError::PaymentMethodNotAvailable;
    }
    if (response.taxiApiError == ETaxiApiError::TariffNotFound) {
        return ESetPaymentMethodError::TariffNotFound;
    }
    return ESetPaymentMethodError::NoValidResponse;
}

TGetPaymentMethodsResult TTaxiApi::GetPaymentMethods(NSc::TValue& methods, const NSc::TValue& location, TStringBuf type) {
    Y_ASSERT(FullInit);
    methods.SetArray();
    if (type && SUPPORTED_PAYMENT_METHODS.find(type) == SUPPORTED_PAYMENT_METHODS.end()) {
        return EGetPaymentMethodsError::NoAvailablePaymentMethods;
    }

    TResponse response;
    NSc::TValue request;
    request["point"] = LocationToCoordsList(location);
    request["user_id"] = TaxiUid;
    request["sourceid"] = ALICE_SOURCE_ID;
    TStringBuf tariff = Tariff->Value["type"].GetString();
    tariff = tariff ? tariff : DEFAULT_TARIFF_TYPE;
    request["class"].SetString(tariff);
    AddAliceParameters(request);

    if (Request(request, response, PAYMENTSMETHODS)) {
        for (const NSc::TValue& responseMethod : response.Data["methods"].GetArray()) {
            if (!type || responseMethod["type"] == type) {
                if (SUPPORTED_PAYMENT_METHODS.find(responseMethod["type"]) != SUPPORTED_PAYMENT_METHODS.end() &&
                    responseMethod["can_order"].GetBool() &&
                    responseMethod["zone_available"].GetBool())
                {
                    NSc::TValue &tmp = methods.Push();
                    tmp["type"] = responseMethod["type"];
                    tmp["id"] = responseMethod["id"];

                    if (responseMethod["type"] == PAYMENT_CARD) {
                        auto cardNum = responseMethod["description"].GetString();
                        tmp["card_number"] = CutCardNumber(cardNum);
                    }
                }
            }
        }
    } else if (response.taxiApiError == ETaxiApiError::TariffNotFound) {
        return EGetPaymentMethodsError::TariffNotFound;
    } else {
        return EGetPaymentMethodsError::NoValidResponse;
    }

    if (methods.ArraySize() == 0) {
        return EGetPaymentMethodsError::NoAvailablePaymentMethods;
    }

    return TGetPaymentMethodsResult();
}

TMakeOrderResult TTaxiApi::MakeOrder(const NSc::TValue& locationFrom, const NSc::TValue& locationTo, TStringBuf comment, TString* orderId) {
    Y_ASSERT(FullInit);
    NSc::TValue request;
    TResponse response;
    request["callcenter"]["key"] = ALICE_SOURCE_ID;
    request["callcenter"]["phone"] = Phone;
    request["yandex_uid"] = YandexUid;
    TStringBuf tariff = Tariff->Value["type"].GetString();
    tariff = tariff ? tariff : DEFAULT_TARIFF_TYPE;
    request["class"].SetArray().Push().SetString(tariff);
    request["comment"].SetString(comment);
    request["corpweb"].SetBool(false);
    request["id"] = TaxiUid;
    request["offer"] = Offer;
    if (!PaymentMethod->Value["type"].IsNull()) {
        request["payment"]["type"] = PaymentMethod->Value["type"];
        if (!PaymentMethod->Value["id"].IsNull()) {
            request["payment"]["payment_method_id"] = PaymentMethod->Value["id"];
        }
    } else {
        request["payment"]["type"].SetString(PAYMENT_CASH);
    }
    request["route"].SetArray();
    request["route"].Push(LocationToTaxiFormat(locationFrom));
    request["route"].Push(LocationToTaxiFormat(locationTo));
    request["parks"].SetArray();
    request["callback"]["data"] = NPushNotification::CreatePushCallbackData(Uuid, DeviceId, YandexUid, ClientId);
    request["callback"]["notify_on"].SetArray();
    for (auto& event : NPushNotification::EVENTS) {
        request["callback"]["notify_on"].Push(event);
    }

    if (Request(request, response, DRAFT)) {
        NSc::TValue request2;
        request2["userid"] = TaxiUid;
        request2["orderid"] = response.Data["orderid"];
        request2["sourceid"] = ALICE_SOURCE_ID;

        if (orderId) {
            *orderId = response.Data["orderid"].GetString();
        }

        response.Data = NSc::TValue();

        if (Request(request2, response, COMMIT)) {
            return TMakeOrderResult();
        }

        if (response.HttpCode == HttpCodes::HTTP_NOT_ACCEPTABLE) {
            return EMakeOrderError::OfferExpired;
        }
    }

    return EMakeOrderError::NoValidResponse;
}

TGetStatusResult TTaxiApi::GetStatus(EOrderStatus* resultStatus, NSc::TValue* resultData) {
    Y_ASSERT(FullInit);
    constexpr size_t lonIndex = 0;
    constexpr size_t latIndex = 1;
    NSc::TValue request;
    TResponse response;
    AddUserInfo(request);
    request["sourceid"] = ALICE_SOURCE_ID;
    if (Request(request, response, SEARCH)) {
        NSc::TValue& order = response.Data["orders"][0];
        if (!order.IsNull()) {
            (*resultData)["orderid"] = order["orderid"];
            (*resultData)["cancel_message"] = order["cancel_rules"]["message"];
            (*resultData)["cancel_state"] = order["cancel_rules"]["state"];
            (*resultData)["cancel_disabled"] = order["cancel_disabled"];
            (*resultData)["time_left"] = order["time_left"];
            (*resultData)["drivers_name"] = order["driver"]["name"];
            (*resultData)["drivers_phone"] = order["driver"]["phone"];
            (*resultData)["car_number"] = order["vehicle"]["plates"];
            (*resultData)["car_short_number"] = order["vehicle"]["short_car_number"];
            (*resultData)["car_model"] = order["vehicle"]["model"];
            (*resultData)["car_color"] = order["vehicle"]["color"];
            (*resultData)["driver_lon"] = order["vehicle"]["location"][lonIndex];
            (*resultData)["driver_lat"] = order["vehicle"]["location"][latIndex];
            if (order["legal_entities"].IsArray()) {
                for (size_t i = 0; i < order["legal_entities"].ArraySize(); ++i) {
                    NSc::TValue& entity = order["legal_entities"][i];
                    if (entity["type"].GetString() == "park") {
                        (*resultData)["partner"] = entity["name"];
                        (*resultData)["partner_ogrn"] = entity["registration_number"];
                        (*resultData)["partner_adress"] = entity["address"];
                    } else if (entity["type"].GetString() == "carrier_permit_owner") {
                        (*resultData)["carrier"] = entity["name"];
                        (*resultData)["carrier_ogrn"] = entity["registration_number"];
                        (*resultData)["carrier_adress"] = entity["address"];
                        (*resultData)["carrier_schedule"] = entity["work_hours"];
                    }
                }
            }
            if (!order["cost_message_details"].IsNull()) {
                if (order["cost_message_details"]["cost_breakdown"].IsArray()) {
                    NSc::TValue& costInfos = order["cost_message_details"]["cost_breakdown"];
                    for (size_t i = 0; i < costInfos.ArraySize(); ++i) {
                        if (costInfos[i]["display_name"].GetString() == "cost") {
                            (*resultData)["price"] = costInfos[i]["display_amount"].GetString();
                        }
                    }
                }
            }

            const NSc::TValue& route = order["request"]["route"];
            constexpr size_t fromIndex = 0;
            constexpr size_t coordsNum = 2;
            constexpr size_t minLocationsNum = 2;
            const size_t toIndex = route.ArraySize() - 1;

            if (route.IsArray() && route.ArraySize() >= minLocationsNum &&
                route[fromIndex]["geopoint"].IsArray() && route[fromIndex]["geopoint"].ArraySize() == coordsNum &&
                route[toIndex]["geopoint"].IsArray() && route[toIndex]["geopoint"].ArraySize() == coordsNum)
            {
                (*resultData)["location_from"]["lat"] = route[fromIndex]["geopoint"][latIndex].ForceNumber();
                (*resultData)["location_from"]["lon"] = route[fromIndex]["geopoint"][lonIndex].ForceNumber();
                (*resultData)["location_to"]["lat"] = route[toIndex]["geopoint"][latIndex].ForceNumber();
                (*resultData)["location_to"]["lon"] = route[toIndex]["geopoint"][lonIndex].ForceNumber();
            } else {
                LOG(ERR) << "Taxi API error: " << "No coordinates found in response" << Endl;
                return EGetStatusError::NoValidResponse;
            }

            if (!TryFromString(order["status"], *resultStatus)) {
                *resultStatus = EOrderStatus::Unknown;
                (*resultData)["unknown_status"] = order["status"];
            }
        } else {
            return EGetStatusError::NoOrdersFound;
        }
    } else {
        return EGetStatusError::NoValidResponse;
    }
    return TGetStatusResult();
}

TCancelResult TTaxiApi::CancelOrder(TStringBuf orderId) {
    Y_ASSERT(FullInit);
    TResponse response;
    NSc::TValue request;
    request["userid"] = TaxiUid;
    request["sourceid"] = ALICE_SOURCE_ID;
    request["orderid"] = orderId;
    request["cancelled_reason"].SetArray().Push("mistaken");

    if (Request(request, response, CANCEL)) {
        return TCancelResult();
    }

    switch (response.HttpCode) {
        case HttpCodes::HTTP_NOT_FOUND:
            return ECancelError::NoOrdersFound;
        case HttpCodes::HTTP_CONFLICT:
            return ECancelError::CanNotCancel;
        default:
            return ECancelError::NoValidResponse;
    }
}

TGetLocationInfoResult TTaxiApi::GetLocationInfo(const NSc::TValue& location) {
    TResponse response;
    NSc::TValue request;
    request["point"] = LocationToCoordsList(location);
    request["sourceid"] = ALICE_SOURCE_ID;
    request["id"] = TaxiUid;

    if (Request(request, response, ZONEINFO)) {
        HasLocationInfo = true;
        for (const NSc::TValue& tariffInfo : response.Data["max_tariffs"].GetArray()) {
            // it's too complex tariff for Alice now
            if (BANNED_TARIFFS.find(tariffInfo["id"].GetString()) == BANNED_TARIFFS.end()) {
                AvailableTariffs[tariffInfo["id"].GetString()] = tariffInfo["name"].GetString();
                AvailableTariffsList.push_back(TString{tariffInfo["id"].GetString()});
            }
        }
        return TGetLocationInfoResult();
    }
    switch (response.HttpCode) {
        case HttpCodes::HTTP_NOT_FOUND:
            return ELocationInfoError::LocationIsNotSupported;
        default:
            return ELocationInfoError::NoValidResponse;
    }
}

TSendMessageToSupportResult TTaxiApi::SendMessageToSupport(TStringBuf message) {
    Y_ASSERT(FullInit);
    TResponse response;
    NSc::TValue request;
    request["user_phone"].SetString(Phone);
    request["text"].SetString(message ? message : DEFAULT_SUPPORT_MESSAGE);
    if (Request(request, response, SUPPORT, true /* isSupport */)) {
        return TSendMessageToSupportResult();
    }
    return ESendMessageToSupportError::NoValidResponse;
}

TSetTariffResult TTaxiApi::SetTariff(TStringBuf tariff) {
    if (!tariff) {
        Tariff->Value["name"] = DEFAULT_TARIFF_NAME;
        Tariff->Value["type"] = DEFAULT_TARIFF_TYPE;
        return TSetTariffResult();
    }

    Y_ASSERT(HasLocationInfo);
    Y_ASSERT(FullInit);
    auto tariffPtr = AvailableTariffs.find(tariff);
    if (tariffPtr == AvailableTariffs.end()) {
        return ESetTariffError::UnsupportedTariff;
    }
    Tariff->Value["name"] = tariffPtr->second;
    Tariff->Value["type"] = tariffPtr->first;
    return TSetTariffResult();
}

void TTaxiApi::GetTariffsList(NSc::TValue& tariffs) {
    for (auto& availableTariffId : AvailableTariffsList) {
        tariffs.Push(AvailableTariffs[availableTariffId]);
    }
}
} // namespace NTaxi
} // namespace NBass
