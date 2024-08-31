#pragma once

#include <alice/bass/util/error.h>

#include <util/generic/maybe.h>

#pragma once
namespace NBASS {
namespace NTaxi {

enum class EConfirmOrderStatus {
    Ok,
    NotConfirmed,
    WrongAddress,
    OpenApp
};

enum class ECancelOrderStatus {
    Ok,
    NotConfirmed
};

enum class EMakeOrderError {
    NoValidResponse,
    OfferExpired
};

enum class ECancelError {
    NoValidResponse,
    CanNotCancel,
    NoOrdersFound
};

enum class EConfigureUserIdError {
    NoValidResponse,
    UserBlocked,
    UserHasDebt,
    InvalidPhoneNumber
};

enum class EGetPriceError {
    NoValidResponse,
    CantConstructRoute,
    LocationIsNotSupported
};

enum class ELocationInfoError {
    NoValidResponse,
    LocationIsNotSupported
};

enum class EOrderStatus {
    Scheduling /* "scheduling" */,
    Scheduled /* "scheduled" */,
    Searching /* "search" */,
    Driving /* "driving" */,
    Waiting /* "waiting" */,
    Transporting /* "transporting" */,
    Complete /* "complete" */,
    Failed /* "failed" */,
    Cancelled /* "cancelled" */,
    Preexpired /* "preexpired" */,
    Expired /* "expired" */,
    Draft /* "draft" */,
    Finished /* "finished" */,
    Unknown /* "unknown" */
};

enum class EGetStatusError {
    NoValidResponse,
    NoOrdersFound
};

enum class ESetPaymentMethodError {
    NoValidResponse,
    PaymentMethodNotAvailable,
    TariffNotFound
};

enum class EGetPaymentMethodsError {
    NoValidResponse,
    NoAvailablePaymentMethods,
    TariffNotFound
};

enum class EScenariosError {
    TaxiNotSupportedOnDevice,
    NeedPhoneInPassportForOrderFromSmartSpeaker,
    HaveNotResolvedLocation,
    NeedApplicationDevice,
    NeedPhoneDevice,
    InvalidSelectedPaymentId,
    BlackBoxError
};

enum class EResolveLocationFromError {
    Fatal,
    GeoNotFound,
    NeedAskGeoPointId,
    BadAccuracy
};

enum class EResolveLocationToError {
    Fatal,
    GeoNotFound
};

enum class ESendMessageToSupportError {
    NoValidResponse
};

enum class ESetTariffError {
    UnsupportedTariff
};

enum class ETaxiApiError {
    TariffNotFound,
    InvalidPhoneNumber
};

using TCancelResult = TMaybe<ECancelError>;
using TConfigureUserIdResult = TMaybe<EConfigureUserIdError>;
using TGetLocationInfoResult = TMaybe<ELocationInfoError>;
using TGetPaymentMethodsResult = TMaybe<EGetPaymentMethodsError>;
using TGetPriceResult = TMaybe<EGetPriceError>;
using TGetStatusResult = TMaybe<EGetStatusError>;
using TMakeOrderResult = TMaybe<EMakeOrderError>;
using TResolveLocationFromResult = TMaybe<EResolveLocationFromError>;
using TResolveLocationToResult = TMaybe<EResolveLocationToError>;
using TSendMessageToSupportResult = TMaybe<ESendMessageToSupportError>;
using TSetPaymentMethodResult = TMaybe<ESetPaymentMethodError>;
using TSetTariffResult = TMaybe<ESetTariffError>;
using TTaxiApiError = TMaybe<ETaxiApiError>;
}  // namespace NTaxi
}  // namespace NBASS
