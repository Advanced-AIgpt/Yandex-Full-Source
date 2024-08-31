#include "handler.h"
#include "integration_api.h"
#include "statuses.h"

#include <alice/bass/forms/common/directives.h>
#include <alice/bass/forms/common/personal_data.h>
#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/geo_resolver.h>
#include <alice/bass/forms/geodb.h>
#include <alice/bass/forms/maps_static_api.h>
#include <alice/bass/forms/route_tools.h>
#include <alice/bass/forms/special_location.h>
#include <alice/bass/forms/urls_builder.h>

#include <alice/bass/libs/client/experimental_flags.h>
#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/metrics/metrics.h>

#include <alice/bass/util/error.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <alice/megamind/protos/analytics/scenarios/taxi/taxi.pb.h>

#include <kernel/geodb/countries.h>
#include <library/cpp/scheme/scheme.h>
#include <util/generic/vector.h>
#include <util/string/cast.h>
#include <alice/bass/forms/remember_address.h>


namespace NBASS::NTaxi {
namespace {
constexpr TStringBuf LEGAL_LINK = "https://yandex.ru/legal/taxi_termsofuse/";
constexpr TStringBuf PASSPORT_PHONE_LINK = "https://passport.yandex.ru/profile/phones";
constexpr TStringBuf AUTH_LINK = "yandex-auth://?theme=light";

// Intents
constexpr TStringBuf TAXI_CALL_TO_DRIVER = "personal_assistant.scenarios.taxi_new_call_to_driver";
constexpr TStringBuf TAXI_CALL_TO_DRIVER_INTERNAL = "personal_assistant.scenarios.taxi_new_call_to_driver_internal";
constexpr TStringBuf TAXI_CALL_TO_SUPPORT = "personal_assistant.scenarios.taxi_new_call_to_support";
constexpr TStringBuf TAXI_CANCEL = "personal_assistant.scenarios.taxi_new_cancel";
constexpr TStringBuf TAXI_CANCEL_CONFIRMATION_YES = "personal_assistant.scenarios.taxi_new_cancel__confirmation_yes";
constexpr TStringBuf TAXI_CANCEL_CONFIRMATION_NO = "personal_assistant.scenarios.taxi_new_cancel__confirmation_no";
constexpr TStringBuf TAXI_DISABLED = "personal_assistant.scenarios.taxi_new_disabled";
constexpr TStringBuf TAXI_LEGAL = "personal_assistant.scenarios.taxi_new_show_legal";
constexpr TStringBuf TAXI_OPEN_APP = "personal_assistant.scenarios.taxi_new_open_app";
constexpr TStringBuf TAXI_ORDER = "personal_assistant.scenarios.taxi_new_order";
constexpr TStringBuf TAXI_ORDER_CONFIRMATION_YES = "personal_assistant.scenarios.taxi_new_order__confirmation_yes";
constexpr TStringBuf TAXI_ORDER_CONFIRMATION_NO = "personal_assistant.scenarios.taxi_new_order__confirmation_no";
constexpr TStringBuf TAXI_ORDER_CONFIRMATION_WRONG = "personal_assistant.scenarios.taxi_new_order__confirmation_wrong";
constexpr TStringBuf TAXI_ORDER_SPECIFY = "personal_assistant.scenarios.taxi_new_order__specify";
constexpr TStringBuf TAXI_SHOW_DRIVER_INFO = "personal_assistant.scenarios.taxi_new_show_driver_info";
constexpr TStringBuf TAXI_STATUS = "personal_assistant.scenarios.taxi_new_status";
constexpr TStringBuf TAXI_CHANGE_PAYMENT_OR_TARIFF = "personal_assistant.scenarios.taxi_new_order__change_payment_or_tariff";
constexpr TStringBuf TAXI_AFTER_ORDER_ACTIONS = "personal_assistant.scenarios.taxi_new_after_order_actions";
constexpr TStringBuf TAXI_STATUS_ADDRESS = "personal_assistant.scenarios.taxi_new_status_address";
constexpr TStringBuf TAXI_STATUS_PRICE = "personal_assistant.scenarios.taxi_new_status_price";
constexpr TStringBuf TAXI_STATUS_TIME = "personal_assistant.scenarios.taxi_new_status_time";
constexpr TStringBuf TAXI_ORDER_SELECT_CARD = "personal_assistant.scenarios.taxi_new_order__select_card";
constexpr TStringBuf TAXI_ORDER_CHANGE_CARD = "personal_assistant.scenarios.taxi_new_order__change_card";

// Slots
constexpr TStringBuf ALLOWED_CARDS = "allowed_cards";
constexpr TStringBuf AVAILABLE_PAYMENT_METHODS = "available_payment_methods";
constexpr TStringBuf AVAILABLE_TARIFFS = "available_tariffs";
constexpr TStringBuf CANCEL_DISABLED = "cancel_disabled";
constexpr TStringBuf CANCEL_MESSAGE = "cancel_message";
constexpr TStringBuf CARD_NUMBER = "card_number";
constexpr TStringBuf CHECKED_PAYMENT_METHOD = "checked_payment_method";
constexpr TStringBuf CHECKED_TARIFF = "checked_tariff";
constexpr TStringBuf CONFIRM = "confirm";
constexpr TStringBuf CONFIRMATION_WRONG = "confirmation_wrong";
constexpr TStringBuf CONNECT_NAMED_LOCATION_TO_DEVICE_CONFIRMATION = "connect_named_location_to_device__confirmation";
constexpr TStringBuf DEEP_LINK = "deep_link";
constexpr TStringBuf ESTIMATE_ROUTE_TIME_MINUTES = "estimate_route_time_minutes";
constexpr TStringBuf IS_ACTIVE_ORDER = "is_active_order";
constexpr TStringBuf LOCATION = "location";
constexpr TStringBuf LOCATION_FROM = "location_from";
constexpr TStringBuf LOCATION_TO = "location_to";
constexpr TStringBuf MAP_URL_SLOT = "map_url";
constexpr TStringBuf OFFER = "offer";
constexpr TStringBuf OPEN_URI = "open_uri";
constexpr TStringBuf ORDER_ID = "orderid";
constexpr TStringBuf PAYMENT_METHOD_SLOT = "payment_method";
constexpr TStringBuf PHONE = "phone";
constexpr TStringBuf RESOLVED_LOCATION_FROM = "resolved_location_from";
constexpr TStringBuf RESOLVED_LOCATION_TO = "resolved_location_to";
constexpr TStringBuf SELECTED_CARD = "selected_card";
constexpr TStringBuf STATUS_SLOT_NAME = "status";
constexpr TStringBuf STATUS_REDIRECT_SLOT_NAME = "status_redirect_form_name";
constexpr TStringBuf STOP_OPTIONS_SLOT = "stop_options";
constexpr TStringBuf TARIFF_SLOT = "tariff";
constexpr TStringBuf TAXI_SEARCH_FAILED = "taxi_search_failed";
constexpr TStringBuf TAXI_PROFILE = "taxi_profile";
constexpr TStringBuf WAITING_TIME_MINUTES = "waiting_time_minutes";
constexpr TStringBuf WHAT_CHANGE = "what_change";
constexpr TStringBuf WHAT_FROM = "what_from";
constexpr TStringBuf WHAT_TO = "what_to";
constexpr TStringBuf WHAT_UNKNOWN = "what_unknown";
constexpr TStringBuf WHERE_FROM = "where_from";
constexpr TStringBuf WHERE_TO = "where_to";
constexpr TStringBuf WHERE_UNKNOWN = "where_unknown";

// Attentions
constexpr TStringBuf CONFIRM_CONFUSED = "confirm_confused";
constexpr TStringBuf FIRST_ORDER = "first_order";
constexpr TStringBuf FROM_ALMOST_EXACT = "from_almost_exact";
constexpr TStringBuf FROM_INCOMPLETE = "from_incomplete";
constexpr TStringBuf LOCATION_IS_NOT_SUPPORTED = "location_is_not_supported";
constexpr TStringBuf NEED_APPLICATION_DEVICE = "need_application_device";
constexpr TStringBuf NEED_PHONE_DEVICE = "need_phone_device";
constexpr TStringBuf NO_ACTIVE_ORDERS = "no_active_orders";
constexpr TStringBuf NO_DRIVER_YET = "no_driver_yet";
constexpr TStringBuf NO_PHONE_NUMBER = "no_phone_number";
constexpr TStringBuf NOT_UNDERSTOOD = "not_understood";
constexpr TStringBuf PAYMENT_METHOD_NOT_AVAILABLE = "payment_method_not_available";
constexpr TStringBuf PHONISH_ATTENTION = "phonish_attention";
constexpr TStringBuf REDIRECT_NOT_FROM_ORDER_INTENT = "redirect_not_from_order_intent";
constexpr TStringBuf REDIRECT_TO_ADD_PHONE_PAGE = "redirect_to_add_phone_page";
constexpr TStringBuf REDIRECT_TO_LOGIN_PAGE = "redirect_to_login_page";
constexpr TStringBuf TARIFF_NOT_AVAILABLE = "tariff_not_available";
constexpr TStringBuf TO_ALMOST_EXACT = "to_almost_exact";
constexpr TStringBuf TO_INCOMPLETE = "to_incomplete";

// Metrics
constexpr TStringBuf METRICS_BAD_USER_LOCATION_ACCURACY = "new_taxi_bad_user_location_accuracy";
constexpr TStringBuf METRICS_CALL_TO_DRIVER = "new_taxi_metrics_call_to_driver";
constexpr TStringBuf METRICS_CALL_TO_DRIVER_NO_DRIVERS_PHONE = "new_taxi_call_to_driver_no_drivers_phone";
constexpr TStringBuf METRICS_CALL_TO_DRIVER_NO_ORDER = "new_taxi_call_to_driver_no_order";
constexpr TStringBuf METRICS_CALL_TO_DRIVER_SUCCESS = "new_taxi_call_to_driver_success";
constexpr TStringBuf METRICS_CALL_TO_DRIVER_TRY_FROM_NOT_PHONE = "new_taxi_call_to_driver_try_from_not_phone";
constexpr TStringBuf METRICS_CALL_TO_SUPPORT = "new_taxi_call_to_support";
constexpr TStringBuf METRICS_CALL_TO_SUPPORT_SUCCESS = "new_taxi_call_to_support_success";
constexpr TStringBuf METRICS_CALL_TO_SUPPORT_TRY_FROM_NOT_PHONE = "new_taxi_call_to_support_try_from_not_phone";
constexpr TStringBuf METRICS_CANCEL = "new_taxi_cancel";
constexpr TStringBuf METRICS_CANCEL_CONFIRM = "new_taxi_cancel_confirm";
constexpr TStringBuf METRICS_CANCEL_CONFIRMED = "new_taxi_cancel_confirmed";
constexpr TStringBuf METRICS_CANCEL_DISABLED = "new_taxi_cancel_disabled";
constexpr TStringBuf METRICS_CANCEL_NO_ACTIVE_ORDER = "new_taxi_cancel_no_active_orders";
constexpr TStringBuf METRICS_CANCEL_NOT_CONFIRMED = "new_taxi_cancel_not_confirmed";
constexpr TStringBuf METRICS_CHANGE_PAYMENT_OR_TARIFF = "new_taxi_change_payment_or_tariff";
constexpr TStringBuf METRICS_CHANGE_CARD = "new_taxi_change_card";
constexpr TStringBuf METRICS_CONFIRMATION_CANCEL = "new_taxi_confirmation_cancel";
constexpr TStringBuf METRICS_CONFIRMATION_CANCEL_BEFORE_OFFER = "new_taxi_confirmation_cancel_before_offer";
constexpr TStringBuf METRICS_CONFIRMATION_FALSE_POSITIVE = "new_taxi_confirmation_false_positive";
constexpr TStringBuf METRICS_ENTER = "new_taxi_enter";
constexpr TStringBuf METRICS_ERROR = "new_taxi_error_";
constexpr TStringBuf METRICS_ESTIMATE_LOCATION_IS_NOT_SUPPORTED = "new_taxi_estimate_location_is_not_supported";
constexpr TStringBuf METRICS_GET_CANCEL_OFFER = "new_taxi_get_cancel_offer";
constexpr TStringBuf METRICS_GET_STATUS = "new_taxi_get_status";
constexpr TStringBuf METRICS_LINK_TO_SITE = "new_taxi_link_to_site";
constexpr TStringBuf METRICS_LINK_TO_SITE_SUCCESS = "new_taxi_link_to_site_success";
constexpr TStringBuf METRICS_OFFER_SUCCESS = "new_taxi_offer_success";
constexpr TStringBuf METRICS_OPEN_APP = "new_taxi_open_app";
constexpr TStringBuf METRICS_OPEN_APP_SUCCESS = "new_taxi_open_app_success";
constexpr TStringBuf METRICS_OPEN_APP_TRY_FROM_NOT_PHONE = "new_taxi_open_app_try_from_not_phone";
constexpr TStringBuf METRICS_ORDER_ALREADY_EXISTS = "new_taxi_order_already_exists";
constexpr TStringBuf METRICS_ORDER_CONFIRM = "new_taxi_order_confirm";
constexpr TStringBuf METRICS_ORDER_SPECIFY = "new_taxi_order_specify";
constexpr TStringBuf METRICS_ORDER_STOP_OPTIONS = "new_taxi_order_stop_options";
constexpr TStringBuf METRICS_RESOLVE_LOCATION_FROM_ERROR = "new_taxi_resolve_location_from_error";
constexpr TStringBuf METRICS_RESOLVE_LOCATION_TO_ERROR = "new_taxi_resolve_location_to_error";
constexpr TStringBuf METRICS_SHOW_DRIVER_INFO = "new_taxi_show_driver_info";
constexpr TStringBuf METRICS_SHOW_LEGAL = "new_taxi_show_legal";
constexpr TStringBuf METRICS_SHOW_LEGAL_SUCCESS = "new_taxi_show_legal_success";
constexpr TStringBuf METRICS_SHOW_LEGAL_TRY_FROM_NOT_PHONE = "new_taxi_show_legal_try_from_not_phone";
constexpr TStringBuf METRICS_STATUS_GET = "new_taxi_status_get";
constexpr TStringBuf METRICS_SUCCESS_CONFIRM = "new_taxi_success_confirm";
constexpr TStringBuf METRICS_TO_ADD_PHONE_PAGE = "new_taxi_to_add_phone_page";
constexpr TStringBuf METRICS_TO_LOGIN_PAGE = "new_taxi_to_login_page";
constexpr TStringBuf METRICS_WRONG_ADDRESS = "new_taxi_wrong_address";
constexpr TStringBuf METRICS_ZONEINFO_LOCATION_IS_NOT_SUPPORTED = "new_taxi_zoneinfo_location_is_not_supported";

// Commands
constexpr TStringBuf CALL_TO_DRIVER_COMMAND = "taxi_call_to_driver_action";
constexpr TStringBuf OPEN_APP_COMMAND = "taxi_order__open_app";
constexpr TStringBuf REDIRECT_TO_PASSPORT_COMMAND = "taxi_redirect_to_passport_action";
constexpr TStringBuf SHOW_LEGAL_COMMAND = "taxi_show_legal_action";

// Suggests
constexpr TStringBuf CALL_TO_DRIVER_SUGGEST = "taxi_call_to_driver";
constexpr TStringBuf CALL_TO_SUPPORT_SUGGEST = "taxi_call_to_support";
constexpr TStringBuf CANCEL_ORDER_SUGGEST = "taxi_cancel_order";
constexpr TStringBuf CANCEL_SUGGEST = "taxi_cancel";
constexpr TStringBuf CHANGE_ADDRESS_SUGGEST = "taxi_change_address";
constexpr TStringBuf CHANGE_PAYMENT_SUGGEST = "taxi_change_payment";
constexpr TStringBuf CHANGE_TARIFF_SUGGEST = "taxi_change_tariff";
constexpr TStringBuf FROM_HOME_SUGGEST = "taxi_from_home";
constexpr TStringBuf FROM_WORK_SUGGEST = "taxi_from_work";
constexpr TStringBuf GET_STATUS_SUGGEST = "taxi_get_status";
constexpr TStringBuf NO_SUGGEST = "taxi_no";
constexpr TStringBuf OK_SUGGEST = "taxi_ok";
constexpr TStringBuf OPEN_IN_APP_SUGGEST = "taxi_open_app";
constexpr TStringBuf ORDER_TAXI_SUGGEST = "taxi_order_taxi";
constexpr TStringBuf SHOW_LEGAL_SUGGEST = "taxi_show_legal";
constexpr TStringBuf TO_HOME_SUGGEST = "taxi_to_home";
constexpr TStringBuf TO_METRO_SUGGEST = "taxi_to_metro";
constexpr TStringBuf TO_WORK_SUGGEST = "taxi_to_work";
constexpr TStringBuf WHO_IS_TRANSPORTER_SUGGEST = "taxi_who_is_transporter";
constexpr TStringBuf YES_SUGGEST = "taxi_yes";

// Types
constexpr TStringBuf BOOL_TYPE = "bool";
constexpr TStringBuf GEO_TYPE = "geo";
constexpr TStringBuf STRING_TYPE = "string";
constexpr TStringBuf ARRAY_TYPE = "array";

// Options
constexpr TStringBuf PAYMENT = "payment";
constexpr TStringBuf TARIFF = "tariff";
constexpr TStringBuf YES = "yes";
constexpr TStringBuf NO = "no";
constexpr TStringBuf WRONG = "wrong";

constexpr double GEO_ACCURACY_THRESHOLD = 100; // meters
constexpr double UNKNOWN_IS_FROM_THRESHOLD = 0.5; // kilometers

// Push types
const TString LOCAL_SEND_LEGAL{TStringBuf("local_send_legal")};
const TString LOCAL_CALL_TO_DRIVER{TStringBuf("local_call_to_driver")};
const TString LOCAL_WHO_IS_TRANSPORTER{TStringBuf("local_who_is_transporter")};
const TString LOCAL_ADD_PHONE_IN_PASSPORT{TStringBuf("local_add_phone_in_passport")};

// History fields
const TString USER_ALREADY_MADE_ORDERS_IN_SMART_SPEAKER{TStringBuf("user_already_made_orders")};
const TString USER_ALREADY_GET_OFFER_IN_SMART_SPEAKER{TStringBuf("user_already_get_offer_in_smart_speaker")};
const TString USER_HAD_DRIVERS_FOR_ORDER_FROM_SMART_SPEAKER{TStringBuf("user_had_drivers")};
const TString USER_ALREADY_MADE_ORDERS_IN_ANY_APP{TStringBuf("user_already_made_orders_in_any_app")};
const TString PREVIOUS_TARIFF{TStringBuf("previous_tariff")};

// fields
constexpr TStringBuf GEO = "geo";

// Geo objects
constexpr TStringBuf AIRPORT = "airport";
constexpr TStringBuf HOUSE = "house";
constexpr TStringBuf METRO = "metro";
constexpr TStringBuf RAILWAY = "railway";
constexpr TStringBuf STREET = "street";
constexpr TStringBuf VEGETATION = "vegetation";
constexpr TStringBuf GEO_OTHER = "other";

// SmartDevicesGeo fiedls
constexpr TStringBuf SMART_DEVICE_LAST_TAXI_ADDRESS = "last_taxi_address";

constexpr TStringBuf EXPERIMENT_TESTING_COMMENT_PREFIX{TStringBuf("taxi_testing_")};

const TVector<TStringBuf> FROM_SLOT_NAMES{WHERE_FROM, WHAT_FROM, RESOLVED_LOCATION_FROM};
const TVector<TStringBuf> TO_SLOT_NAMES{WHERE_TO, WHAT_TO, RESOLVED_LOCATION_TO};
const TVector<TStringBuf> UNKNOWN_SLOT_NAMES{WHERE_UNKNOWN, WHAT_UNKNOWN};
const TVector<TStringBuf> GEO_OBJECTS_EXACT{AIRPORT, HOUSE, METRO, RAILWAY};
const TVector<TStringBuf> GEO_OBJECTS_ALMOST_EXACT{AIRPORT, HOUSE, METRO, RAILWAY, STREET, VEGETATION, GEO_OTHER};

constexpr ui16 MAP_IMAGE_WIDTH = 450;
constexpr ui16 MAP_IMAGE_HEIGHT = 300;
constexpr ui16 MAP_WAITING_SCALE = 17;

const TSpecialLocation DEFAULT_LOCATION = TSpecialLocation(TSpecialLocation::EType::HOME);

TSlot* GetFirstNonEmptySlot(TContext& ctx, const TVector<TStringBuf>& fields) {
    for (auto& name : fields) {
        auto slot = ctx.GetSlot(name);
        if (!IsSlotEmpty(slot)) {
            return slot;
        }
    }
    return nullptr;
}

void AddAttention(TStringBuf attentionType, TContext& ctx, bool countStats = false) {
    ctx.AddAttention(attentionType);
    if (countStats) {
        Y_STATS_INC_INTEGER_COUNTER(attentionType);
    }
}

const NSc::TValue& GetLocationAddress(const NSc::TValue& location) {
    return location.Has(GEO) ? location[GEO] : location;
}

bool CheckGeoAccuracy(const NSc::TValue& location, const TVector<TStringBuf>& fields) {
    if (!location["company_name"].IsNull()) {
        // In geosearch, some companies do not have a street. For example metro station Mitino.
        // Just assert if it's company then it's exact
        return true;
    }

    const NSc::TValue& locationData = GetLocationAddress(location);
    for (TStringBuf field : fields) {
        if (locationData.Has(field)) {
            return true;
        }
    }
    return false;
}

bool IsExactGeo(const NSc::TValue& location) {
    return CheckGeoAccuracy(location, GEO_OBJECTS_EXACT);
}

bool IsAlmostExactGeo(const NSc::TValue& location) {
    return CheckGeoAccuracy(location, GEO_OBJECTS_ALMOST_EXACT);
}

TString GetDeviceKey(const TContext& ctx) {
    return TStringBuilder{} << ctx.GetDeviceModel() << ctx.GetDeviceId();
}

TResolveLocationFromResult ResolveLocationFrom(TContext& ctx, TRouteResolver& routeResolver,
        NSc::TValue& locationFrom, NSc::TValue& anotherFrom, TResultValue datasyncError,
        NSc::TValue& smartDeviceGeoData, NSc::TValue& history)
{
    TSlot* slotInputResolvedFrom = ctx.GetOrCreateSlot(RESOLVED_LOCATION_FROM, GEO_TYPE);
    TSlot* slotWhatFrom = ctx.GetOrCreateSlot(WHAT_FROM, STRING_TYPE);
    TSlot* slotWhereFrom = ctx.GetOrCreateSlot(WHERE_FROM, STRING_TYPE);
    if (!IsSlotEmpty(slotInputResolvedFrom) && slotInputResolvedFrom->Value.Has(LOCATION)) {
        locationFrom = slotInputResolvedFrom->Value;
    } else {
        // We need to recheck payment method and tariff if change location
        ctx.GetOrCreateSlot(CHECKED_PAYMENT_METHOD, STRING_TYPE)->Value.SetNull();
        ctx.GetOrCreateSlot(CHECKED_TARIFF, STRING_TYPE)->Value.SetNull();

        NSc::TValue& deviceHistory = history[GetDeviceKey(ctx)];

        bool connectDeviceToLocation = ctx.MetaClientInfo().IsSmartSpeaker();
        // DIALOG-4838 first part
        if (connectDeviceToLocation) {
            if (IsSlotEmpty(slotWhatFrom) && IsSlotEmpty(slotWhereFrom)) {
                // if we have connection to location or saved last address, we use it
                if (smartDeviceGeoData.IsNull()) {
                    if (!datasyncError || *datasyncError == TError::EType::NODATASYNCKEYFOUND) {
                        return EResolveLocationFromError::NeedAskGeoPointId;
                    }
                } else if (TStringBuf smartDeviceLocation = smartDeviceGeoData[LOCATION].GetString()) {
                    slotWhatFrom->Value.SetString(smartDeviceLocation);
                } else if (!deviceHistory[SMART_DEVICE_LAST_TAXI_ADDRESS].IsNull()) {
                    slotInputResolvedFrom->Value.CopyFrom(deviceHistory[SMART_DEVICE_LAST_TAXI_ADDRESS]);
                    locationFrom = slotInputResolvedFrom->Value;
                    return TResolveLocationFromResult();
                }
            }
        }

        if (IsSlotEmpty(slotWhatFrom) && IsSlotEmpty(slotWhereFrom)) {
            if (ctx.Meta().Location().Accuracy() > GEO_ACCURACY_THRESHOLD) {
                return EResolveLocationFromError::BadAccuracy;
            }
        }

        if (const TResultValue err = routeResolver.ResolveLocationFrom(&locationFrom, &anotherFrom)) {
            return EResolveLocationFromError::Fatal;
        }

        if (locationFrom.IsNull()) {
            // Failed to resolve location_from, error block was already added
            return EResolveLocationFromError::GeoNotFound;
        }

        // DIALOG-4838 second part
        if (connectDeviceToLocation) {
            // we have to save last location
            if (IsSlotEmpty(slotWhatFrom) && IsSlotEmpty(slotWhereFrom) && !deviceHistory[SMART_DEVICE_LAST_TAXI_ADDRESS].GetString()) {
                const NSc::TValue& locationData = GetLocationAddress(locationFrom);
                if (locationData["house"].GetString()) {
                    deviceHistory[SMART_DEVICE_LAST_TAXI_ADDRESS] = locationFrom;
                } else {
                    return EResolveLocationFromError::BadAccuracy;
                }
            } else {
                deviceHistory[SMART_DEVICE_LAST_TAXI_ADDRESS] = locationFrom;
            }
        }
    }
    slotWhatFrom->Value.Clear();
    slotWhereFrom->Value.Clear();
    return TResolveLocationFromResult();
}

TResolveLocationFromResult ResolveLocationFromOld(TContext& ctx, TRouteResolver& routeResolver,
        NSc::TValue& locationFrom, NSc::TValue& anotherFrom)
{
    TSlot* slotInputResolvedFrom = ctx.GetOrCreateSlot(RESOLVED_LOCATION_FROM, GEO_TYPE);
    TSlot* slotWhatFrom = ctx.GetOrCreateSlot(WHAT_FROM, STRING_TYPE);
    TSlot* slotWhereFrom = ctx.GetOrCreateSlot(WHERE_FROM, STRING_TYPE);

    if (!IsSlotEmpty(slotInputResolvedFrom) && slotInputResolvedFrom->Value.Has(LOCATION)) {
        locationFrom = slotInputResolvedFrom->Value;
    } else if (!IsSlotEmpty(slotWhatFrom) || !IsSlotEmpty(slotWhereFrom)) {
        if (const TResultValue err = routeResolver.ResolveLocationFrom(&locationFrom, &anotherFrom)) {
            return EResolveLocationFromError::Fatal;
        }

        if (locationFrom.IsNull()) {
            // Failed to resolve location_from, error block was already added
            return EResolveLocationFromError::GeoNotFound;
        }
    }
    return TResolveLocationFromResult();
}

TResolveLocationToResult ResolveLocationTo(TContext& ctx, TRouteResolver& routeResolver, const NSc::TValue& locationFrom,
        NSc::TValue& locationTo, NSc::TValue& anotherTo)
{
    TSlot* slotInputResolvedTo = ctx.GetOrCreateSlot(RESOLVED_LOCATION_TO, GEO_TYPE);
    TSlot* slotWhatTo = ctx.GetOrCreateSlot(WHAT_TO, STRING_TYPE);
    TSlot* slotWhereTo = ctx.GetOrCreateSlot(WHERE_TO, STRING_TYPE);
    if (!IsSlotEmpty(slotInputResolvedTo) && slotInputResolvedTo->Value.Has(LOCATION)) {
        locationTo = slotInputResolvedTo->Value;
    } else if (!IsSlotEmpty(slotWhatTo) || !IsSlotEmpty(slotWhereTo)) {
        if (locationFrom.Has(LOCATION)) {
            // Use resolved location_from as the start point for location_to
            const NSc::TValue& location = locationFrom[LOCATION];
            routeResolver.FromPosition = TGeoPosition(location["lat"].GetNumber(), location["lon"].GetNumber());
        }
        if (const TResultValue err = routeResolver.ResolveLocationTo(&locationTo, &anotherTo)) {
            return EResolveLocationToError::Fatal;
        }

        if (locationTo.IsNull()) {
            // Failed to resolve location_to, error block was already added
            return EResolveLocationToError::GeoNotFound;
        }
    }
    slotWhatTo->Value.Clear();
    slotWhereTo->Value.Clear();
    return TResolveLocationToResult();
}

TResolveLocationToResult ResolveLocationUnknown(TContext& ctx, TRouteResolver& routeResolver, const NSc::TValue& locationFrom,
        NSc::TValue& locationUnknown, NSc::TValue& anotherUnknown)
{
    TSlot* slotWhat = ctx.GetOrCreateSlot(WHAT_UNKNOWN, STRING_TYPE);
    TSlot* slotWhere = ctx.GetOrCreateSlot(WHERE_UNKNOWN, STRING_TYPE);
    TString nearest{TSpecialLocation(TSpecialLocation::EType::NEAREST).AsString()};
    if (IsSlotEmpty(slotWhere)) {
        // if not specified where, we need to choose nearest object
        slotWhere->Value.SetString(nearest);
        slotWhere->Type = TStringBuf("special_location");
    }

    if (locationFrom.Has(LOCATION)) {
        // Use resolved location_from as the start point for location_unknown
        const NSc::TValue& location = locationFrom[LOCATION];
        routeResolver.FromPosition = TGeoPosition(location["lat"].GetNumber(), location["lon"].GetNumber());
    }

    if (const TResultValue err = routeResolver.ResolveLocationUnknown(&locationUnknown, &anotherUnknown)) {
        return EResolveLocationToError::Fatal;
    }

    if (locationUnknown.IsNull()) {
        // Failed to resolve location_unknown, error block was already added
        if (slotWhere->Value.GetString() == nearest) {
            // we added this earlier
            slotWhere->Value.Clear();
        }
        return EResolveLocationToError::GeoNotFound;
    }
    slotWhat->Value.Clear();
    slotWhere->Value.Clear();
    return TResolveLocationToResult();
}

void AddSearchAndOnboardingSuggests(TContext& ctx) {
    ctx.AddSearchSuggest();
    ctx.AddOnboardingSuggest();
}

template <class TErr>
TContext::TPtr HandleCustomError(TContext& ctx, const TErr& err, TStringBuf functionName, const TVector<TStringBuf>& suggests = {}, bool addSearchAndOnboarding = false) {
    // No need to run Response Form Handler
    TContext::TPtr newContext = ctx.SetResponseForm(TAXI_DISABLED, false /* setCurrentFormAsCallback */);
    Y_ENSURE(newContext);
    TString errStr = ToString(err);
    NSc::TValue errorData;
    errorData["code"].SetString(errStr);
    AddAttention(TStringBuilder{} << METRICS_ERROR << functionName << '_' << errStr, *newContext, true /* count stats */);
    newContext->AddErrorBlock(
            TError(TError::EType::TAXIERROR, TStringBuilder{} << functionName << " error: " << errStr),
            errorData
        );

    for (auto suggestName : suggests) {
        newContext->AddSuggest(suggestName);
    }

    if (addSearchAndOnboarding) {
        AddSearchAndOnboardingSuggests(*newContext);
    }

    TryAddShowPromoDirective(*newContext);

    return newContext;
}

void CopySlot(const TContext::TSlot& oldSlot, TStringBuf newSlotName, NSc::TValue* formUpdate) {
    TContext::TSlot newSlot(newSlotName, oldSlot.Type);
    newSlot.Value = oldSlot.Value;
    formUpdate->GetOrAdd("slots").SetArray().Push(newSlot.ToJson());
}

bool IsActiveOrder(EOrderStatus status) {
    switch (status) {
        case EOrderStatus::Searching:
        case EOrderStatus::Driving:
        case EOrderStatus::Waiting:
        case EOrderStatus::Transporting:
        case EOrderStatus::Scheduled:
        case EOrderStatus::Scheduling:
            return true;
        case EOrderStatus::Complete:
        case EOrderStatus::Failed:
        case EOrderStatus::Cancelled:
        case EOrderStatus::Preexpired:
        case EOrderStatus::Expired:
        case EOrderStatus::Draft:
        case EOrderStatus::Finished:
        case EOrderStatus::Unknown:
            return false;
    }
}

bool IsKnownDriver(EOrderStatus status) {
    switch (status) {
        case EOrderStatus::Driving:
        case EOrderStatus::Waiting:
        case EOrderStatus::Transporting:
            return true;
        case EOrderStatus::Scheduled:
        case EOrderStatus::Scheduling:
        case EOrderStatus::Searching:
        case EOrderStatus::Complete:
        case EOrderStatus::Failed:
        case EOrderStatus::Cancelled:
        case EOrderStatus::Expired:
        case EOrderStatus::Preexpired:
        case EOrderStatus::Draft:
        case EOrderStatus::Finished:
        case EOrderStatus::Unknown:
            return false;
    }
}

void FillStatusForm(TContext& ctx, EOrderStatus status, NSc::TValue& orderData) {
    auto locationTo = TGeoPosition(orderData[LOCATION_TO]["lat"], orderData[LOCATION_TO]["lon"]);
    auto locationFrom = TGeoPosition(orderData[LOCATION_FROM]["lat"], orderData[LOCATION_FROM]["lon"]);
    ctx.GetOrCreateSlot(STATUS_SLOT_NAME, STRING_TYPE)->Value = ToString(status);
    ctx.GetOrCreateSlot(ORDER_ID, STRING_TYPE)->Value = orderData[ORDER_ID];
    // order_data type - just the type I came up with.
    ctx.GetOrCreateSlot("order_data", "order_data")->Value.CopyFrom(orderData);
    ctx.GetOrCreateSlot("is_active_order", "is_active_order")->Value.SetBool(IsActiveOrder(status));
    ctx.GetOrCreateSlot(LOCATION_TO, GEO_TYPE)->Value = LocationToGeo(ctx, locationTo);
    ctx.GetOrCreateSlot(LOCATION_FROM, GEO_TYPE)->Value = LocationToGeo(ctx, locationFrom);
}

bool IsTaxiAvailable(TTaxiApi& taxiApi, const NSc::TValue& location) {
    // GetLocationInfo returns TMaybe<EGetLocationInfoError>
    return location.IsNull() || !taxiApi.GetLocationInfo(location);
}

bool IsRussianRegion(const NGeobase::TLookup& geobase, NGeobase::TId region) {
    // TODO(ar7is7): REPLACE IT with taxi api function, when it will be ready.
    return !NAlice::IsValidId(region) || geobase.IsIdInRegion(region, NGeoDB::RUSSIA_ID);
}

NGeobase::TId GetLocationGeoId(const NSc::TValue& location) {
    if (location.IsNull()) {
        return NGeobase::UNKNOWN_REGION;
    }

    const NSc::TValue& geoLocation = GetLocationAddress(location);
    i64 geoid = geoLocation["geoid"].GetIntNumber(NGeobase::UNKNOWN_REGION);
    return static_cast<NGeobase::TId>(geoid);
}

void AddFromSuggests(TContext& ctx) {
    ctx.AddSuggest(FROM_HOME_SUGGEST);
    ctx.AddSuggest(FROM_WORK_SUGGEST);
}

void AddToSuggests(TContext& ctx) {
    ctx.AddSuggest(TO_HOME_SUGGEST);
    ctx.AddSuggest(TO_WORK_SUGGEST);
    ctx.AddSuggest(TO_METRO_SUGGEST);
}

void AskWhereFrom(TContext& ctx) {
    ctx.CreateSlot(WHERE_FROM, STRING_TYPE, false /*optional*/);
    ctx.CreateSlot(WHAT_FROM, STRING_TYPE, true /*optional*/);
    ctx.CreateSlot(RESOLVED_LOCATION_FROM, STRING_TYPE, true /*optional*/);
    AddFromSuggests(ctx);
}

void AskWhereTo(TContext& ctx) {
    ctx.CreateSlot(WHERE_TO, STRING_TYPE, false /*optional*/);
    ctx.CreateSlot(WHAT_TO, STRING_TYPE, true /*optional*/);
    ctx.CreateSlot(RESOLVED_LOCATION_TO, STRING_TYPE, true /*optional*/);
    AddToSuggests(ctx);
}
bool IsPhoneDevice(TContext& ctx) {
    // check that the device can make calls
    return ctx.MetaClientInfo().IsAndroid() || ctx.MetaClientInfo().IsIOS();
}

bool IsDeviceWithBrowser(TContext& ctx) {
    // check that the device can open sites or apps
    return IsPhoneDevice(ctx) || ctx.MetaClientInfo().IsDesktop();
}

void AddPhoneSuggest(TContext& ctx, TStringBuf suggest) {
    if (IsPhoneDevice(ctx)) {
        ctx.AddSuggest(suggest);
    }
}

void AddBrowserSuggest(TContext& ctx, TStringBuf suggest) {
    if (IsDeviceWithBrowser(ctx)) {
        ctx.AddSuggest(suggest);
    }
}

void StopListeningIfSmartSpeaker(TContext& ctx) {
    if (!IsDeviceWithBrowser(ctx)) {
        ctx.AddStopListeningBlock();
    }
}

void CopyImportantSlotsValues(TContext& ctxFrom, TContext& ctxTo) {
    std::initializer_list<TStringBuf> slots = {
        CHECKED_PAYMENT_METHOD,
        CHECKED_TARIFF,
        PAYMENT_METHOD_SLOT,
        RESOLVED_LOCATION_FROM,
        RESOLVED_LOCATION_TO,
        TARIFF_SLOT,
        TAXI_PROFILE,
        WHAT_FROM,
        WHAT_TO,
        WHERE_FROM,
        WHERE_TO,
    };
    ctxTo.CopySlotsFrom(ctxFrom, slots);
}

TContext::TPtr HandleWrongAddress(TContext& ctx, TSlot* confirmSlot, bool reaskAddresses = true) {
    TContext::TPtr newContext = ctx.SetResponseForm(TAXI_ORDER_SPECIFY, false /* setCurrentFormAsCallback */);
    newContext->GetOrCreateSlot(STATUS_SLOT_NAME, STRING_TYPE)->Value = ToString(EConfirmOrderStatus::WrongAddress);

    bool isFromEmpty = IsSlotEmpty(GetFirstNonEmptySlot(ctx, FROM_SLOT_NAMES));
    bool isToEmpty = IsSlotEmpty(GetFirstNonEmptySlot(ctx, TO_SLOT_NAMES));

    if (isFromEmpty && isToEmpty || reaskAddresses) {
        AskWhereFrom(*newContext);
        // used in MakeOffer
        newContext->CreateSlot(CONFIRMATION_WRONG, STRING_TYPE, false /*optional*/)->Value = confirmSlot->Value;
    } else {
        CopyImportantSlotsValues(ctx, *newContext);
        if (isFromEmpty) {
            AskWhereFrom(*newContext);
        }

        if (isToEmpty) {
            AskWhereTo(*newContext);
        }
    }
    return newContext;
}

TResultValue GetTaxiHistory(TPersonalDataHelper& personalData, const TPersonalDataHelper::TUserInfo& blackBoxInfo, NSc::TValue& history) {
    return personalData.GetDataSyncJsonValue(blackBoxInfo.GetUid(), TPersonalDataHelper::EUserSpecificKey::TaxiHistory, history);
}

TResultValue SaveTaxiHistory(TPersonalDataHelper& personalData, const TPersonalDataHelper::TUserInfo& blackBoxInfo, const NSc::TValue& history) {
    return personalData.SaveDataSyncJsonValue(blackBoxInfo.GetUid(), TPersonalDataHelper::EUserSpecificKey::TaxiHistory, history);
}

TResultValue GetSmartDeviceGeoData(TPersonalDataHelper& personalData, const TPersonalDataHelper::TUserInfo& blackBoxInfo, NSc::TValue& geo_data) {
    return personalData.GetDataSyncJsonValue(blackBoxInfo.GetUid(), TPersonalDataHelper::EUserDeviceSpecificKey::Location, geo_data);
}

TResultValue SaveSmartDeviceGeoData(TPersonalDataHelper& personalData, const TPersonalDataHelper::TUserInfo& blackBoxInfo, const NSc::TValue& geo_data) {
    return personalData.SaveDataSyncJsonValue(blackBoxInfo.GetUid(), TPersonalDataHelper::EUserDeviceSpecificKey::Location, geo_data);
}

double DegreesToRadians(double angle) {
    static constexpr double pi = 3.1415926535;
    static constexpr double degreePerHalfCircle = 180;
    return angle / degreePerHalfCircle * pi;
}

double CalcEarthDistance(const NSc::TValue& firstPoint, const NSc::TValue& secondPoint) {
    static constexpr double earthRadius = 6371.;

    const double lat1 = DegreesToRadians(firstPoint["lat"].GetNumber());
    const double lon1 = DegreesToRadians(firstPoint["lon"].GetNumber());
    const double lat2 = DegreesToRadians(secondPoint["lat"].GetNumber());
    const double lon2 = DegreesToRadians(secondPoint["lon"].GetNumber());

    const double u = sin((lat1 - lat2) / 2.);
    const double v = sin((lon1 - lon2) / 2.);
    return 2. * earthRadius * asin(sqrt(u * u + cos(lat1) * cos(lat2) * v * v));
}
} // namespace

TResultValue THandler::Do(TRequestHandler& r) {
    auto& ctx = r.Ctx();
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::TAXI);

    const auto& client = ctx.MetaClientInfo();
    auto formName = ctx.FormName();
    if (
        (
            client.IsYaAuto() ||
            client.IsElariWatch() ||
            client.IsWeatherPlugin() ||
            client.IsYaMusic() ||
            client.IsMiniSpeakerDexp() ||
            client.IsMiniSpeakerLG() ||
            client.IsYaModule() ||
            client.IsDesktop() && !ctx.HasExpFlag(EXPERIMENTAL_FLAG_TAXI_NEW_DESKTOP) ||
            client.IsWebtouch()
        ) &&
        !ctx.ClientFeatures().SupportsSynchronizedPush()
    ) {
        auto newCtx = HandleCustomError(
            ctx,
            EScenariosError::TaxiNotSupportedOnDevice,
            TStringBuf("taxi_not_supported_on_device")
        );
        newCtx->AddOnboardingSuggest();
        return TResultValue();
    }
    TResultValue result;
    TPersonalDataHelper::TUserInfo blackBoxInfo;
    TPersonalDataHelper personalData(ctx, ctx.HasExpFlag(EXPERIMENTAL_FLAG_TAXI_USE_TEST_BLACKBOX));

    auto stopOptions = ctx.GetOrCreateSlot(STOP_OPTIONS_SLOT, STRING_TYPE);
    auto tariff = ctx.GetOrCreateSlot(TARIFF_SLOT, STRING_TYPE);

    if (BANNED_TARIFFS.find(tariff->Value.GetString()) != BANNED_TARIFFS.end()) {
        stopOptions->Value = tariff->Value;
    }

    if (formName == TAXI_OPEN_APP || formName == TAXI_AFTER_ORDER_ACTIONS) {
        AddAttention(METRICS_OPEN_APP, ctx, true /*count stats*/);
        result = OpenApp(ctx);
    } else if (!IsSlotEmpty(stopOptions)) {
        if (!IsDeviceWithBrowser(ctx)) {
            HandleCustomError(
                ctx,
                EScenariosError::NeedApplicationDevice,
                TStringBuf("stop_options_not_in_phone")
            );
        } else {
            AddAttention(METRICS_LINK_TO_SITE, ctx, true /*count stats*/);
            result = GenLinkToSite(ctx);
        }
    } else if (bool blackBoxResult; !ctx.IsAuthorizedUser() || !(blackBoxResult = personalData.GetUserInfo(blackBoxInfo)) || !blackBoxInfo.GetPhone()) {
        TContext::TPtr newContext;
        if (ctx.IsAuthorizedUser() && !blackBoxResult) {
            newContext = HandleCustomError(
                ctx,
                EScenariosError::BlackBoxError,
                TStringBuf("black_box_wrong_response")
            );
        } else if (client.IsSmartSpeaker()) { // suppose that SmartSpeakers users cant be unauthorized
            TTaxiApi taxiApi{ctx, blackBoxInfo, OFFER, CHECKED_PAYMENT_METHOD, CHECKED_TARIFF, CARD_NUMBER};
            taxiApi.SendPushes(LOCAL_ADD_PHONE_IN_PASSPORT);
            newContext = HandleCustomError(
                ctx,
                EScenariosError::NeedPhoneInPassportForOrderFromSmartSpeaker,
                TStringBuf("no_phone_number_in_smart_speaker")
            );
        } else {
            newContext = ctx.SetResponseForm(TAXI_DISABLED, false /* setCurrentFormAsCallback */);
            NSc::TValue taxiOrderData;

            if (formName != TAXI_ORDER && formName != TAXI_ORDER_SPECIFY) {
                AddAttention(REDIRECT_NOT_FROM_ORDER_INTENT, *newContext);
            }

            if (ctx.IsAuthorizedUser()) {
                AddAttention(METRICS_TO_ADD_PHONE_PAGE, *newContext, true /*count stats*/);
                AddAttention(REDIRECT_TO_ADD_PHONE_PAGE, *newContext);
                taxiOrderData["uri"].SetString(PASSPORT_PHONE_LINK);
            } else {
                AddAttention(METRICS_TO_LOGIN_PAGE, *newContext, true /*count stats*/);
                AddAttention(REDIRECT_TO_LOGIN_PAGE, *newContext);

                if (client.IsSearchApp() || client.IsNavigator() || client.IsYaLauncher()) {
                    taxiOrderData["uri"].SetString(AUTH_LINK);
                } else if (client.IsYaBrowserMobile() || client.IsYaBrowser()) {
                    taxiOrderData["uri"].SetString(PASSPORT_PHONE_LINK);
                } else {
                    // we do not know how to redirect user to login page
                    AddSearchAndOnboardingSuggests(*newContext);
                    return result;
                }
            }
            newContext->AddCommand<TTaxiRedirectToPassportDirective>(OPEN_URI, taxiOrderData);
            newContext->AddSuggest(REDIRECT_TO_PASSPORT_COMMAND, taxiOrderData);
            newContext->AddSuggest(OPEN_IN_APP_SUGGEST);
        }

        AddSearchAndOnboardingSuggests(*newContext);
    } else {
        // It's cache for taxi profile data
        TContext::TSlot* taxiProfileSlot = ctx.GetOrCreateSlot(TAXI_PROFILE, STRING_TYPE);

        TTaxiApi taxiApi{ctx, blackBoxInfo, OFFER, CHECKED_PAYMENT_METHOD, CHECKED_TARIFF, CARD_NUMBER};
        if (TConfigureUserIdResult err = taxiApi.ConfigureUserId(taxiProfileSlot->Value)) {
            HandleCustomError(ctx, *err, TStringBuf("configure_user_id"), {CALL_TO_SUPPORT_SUGGEST, ORDER_TAXI_SUGGEST}, true /*addSearchAndOnboardingSuggests*/);
            return TResultValue();
        }

        // phonish it is type of auth in taxi by phone number only, without yandex account
        if (taxiProfileSlot->Value["is_phonish"].GetBool()) {
            AddAttention(PHONISH_ATTENTION, ctx);
        }

        auto phoneSlot = ctx.GetOrCreateSlot(PHONE, STRING_TYPE);
        phoneSlot->Value = blackBoxInfo.GetPhone();
        ctx.MarkSensitive(*phoneSlot);

        if (formName == TAXI_ORDER) {
            AddAttention(METRICS_ENTER, ctx, true /*count stats*/);
            result = MakeOffer(ctx, taxiApi, personalData, blackBoxInfo);
        } else if (formName == TAXI_ORDER_SPECIFY) {
            AddAttention(METRICS_ORDER_SPECIFY, ctx);
            result = MakeOffer(ctx, taxiApi, personalData, blackBoxInfo);
        } else if (formName == TAXI_ORDER_CONFIRMATION_YES) {
            ctx.GetOrCreateSlot(CONFIRM, STRING_TYPE)->Value.SetString(YES);
            AddAttention(METRICS_ORDER_CONFIRM, ctx);
            result = ConfirmOrder(ctx, taxiApi, personalData, blackBoxInfo);
        } else if (formName == TAXI_ORDER_CONFIRMATION_NO) {
            result = CancelOrderIfExists(ctx, taxiApi, personalData, blackBoxInfo);
        } else if (formName == TAXI_ORDER_CONFIRMATION_WRONG) {
            ctx.GetOrCreateSlot(CONFIRM, STRING_TYPE)->Value.SetString(WRONG);
            AddAttention(METRICS_ORDER_CONFIRM, ctx);
            result = ConfirmOrder(ctx, taxiApi, personalData, blackBoxInfo);
        } else if (formName == TAXI_STATUS || formName == TAXI_STATUS_ADDRESS) {
            AddAttention(METRICS_GET_STATUS, ctx);
            result = GetOrderStatus(ctx, taxiApi, personalData, blackBoxInfo);
        } else if (formName == TAXI_STATUS_TIME || formName == TAXI_STATUS_PRICE) {
            // sorry for that, but to avoid it, I need move to megamind and get a "scenario state"
            TContext::TPtr newContext = ctx.SetResponseForm(TAXI_ORDER, false /* setCurrentFormAsCallback */);
            newContext->GetOrCreateSlot(STATUS_REDIRECT_SLOT_NAME, STRING_TYPE)->Value.SetString(ctx.FormName());
            ctx.RunResponseFormHandler();
        } else if (formName == TAXI_SHOW_DRIVER_INFO) {
            AddAttention(METRICS_SHOW_DRIVER_INFO, ctx);
            result = GetOrderStatus(ctx, taxiApi, personalData, blackBoxInfo);
        } else if (formName == TAXI_CANCEL) {
            AddAttention(METRICS_CANCEL, ctx);
            result = GetCancelOffer(ctx, taxiApi);
        } else if (formName == TAXI_CANCEL_CONFIRMATION_YES) {
            ctx.GetOrCreateSlot(CONFIRM, STRING_TYPE)->Value.SetString(YES);
            AddAttention(METRICS_CANCEL_CONFIRM, ctx);
            result = CancelOrder(ctx, taxiApi);
        } else if (formName == TAXI_CANCEL_CONFIRMATION_NO) {
            ctx.GetOrCreateSlot(CONFIRM, STRING_TYPE)->Value.SetString(NO);
            AddAttention(METRICS_CANCEL_CONFIRM, ctx);
            result = CancelOrder(ctx, taxiApi);
        } else if (formName == TAXI_CALL_TO_SUPPORT) {
            AddAttention(METRICS_CALL_TO_SUPPORT, ctx, true /*count stats*/);
            result = CallToSupport(ctx, taxiApi);
        } else if (formName == TAXI_CALL_TO_DRIVER || formName == TAXI_CALL_TO_DRIVER_INTERNAL) {
            AddAttention(METRICS_CALL_TO_DRIVER, ctx);
            result = CallToDriver(ctx, taxiApi);
        } else if (formName == TAXI_LEGAL) {
            AddAttention(METRICS_SHOW_LEGAL, ctx);
            result = ShowLegal(ctx, taxiApi);
        } else if (formName == TAXI_CHANGE_PAYMENT_OR_TARIFF) {
            AddAttention(METRICS_CHANGE_PAYMENT_OR_TARIFF, ctx);
            result = ChangePaymentOrTariff(ctx, taxiApi);
        } else if (formName == TAXI_ORDER_CHANGE_CARD) {
            AddAttention(METRICS_CHANGE_CARD, ctx);
            result = ChangeCard(ctx, taxiApi);
        } else if (formName == TAXI_ORDER_SELECT_CARD) {
            result = SelectCard(ctx);
        }
    }

    AddSearchAndOnboardingSuggests(ctx);
    return result;
}

void THandler::Register(THandlersMap* handlers) {
    auto cbTaxiOrderForm = []() {
        return MakeHolder<THandler>();
    };

    handlers->emplace(TAXI_ORDER, cbTaxiOrderForm);
    handlers->emplace(TAXI_ORDER_SPECIFY, cbTaxiOrderForm);
    handlers->emplace(TAXI_ORDER_CONFIRMATION_YES, cbTaxiOrderForm);
    handlers->emplace(TAXI_ORDER_CONFIRMATION_NO, cbTaxiOrderForm);
    handlers->emplace(TAXI_ORDER_CONFIRMATION_WRONG, cbTaxiOrderForm);
    handlers->emplace(TAXI_CANCEL, cbTaxiOrderForm);
    handlers->emplace(TAXI_CANCEL_CONFIRMATION_YES, cbTaxiOrderForm);
    handlers->emplace(TAXI_CANCEL_CONFIRMATION_NO, cbTaxiOrderForm);
    handlers->emplace(TAXI_STATUS, cbTaxiOrderForm);
    handlers->emplace(TAXI_LEGAL, cbTaxiOrderForm);
    handlers->emplace(TAXI_OPEN_APP, cbTaxiOrderForm);
    handlers->emplace(TAXI_CALL_TO_SUPPORT, cbTaxiOrderForm);
    handlers->emplace(TAXI_CALL_TO_DRIVER, cbTaxiOrderForm);
    handlers->emplace(TAXI_CALL_TO_DRIVER_INTERNAL, cbTaxiOrderForm);
    handlers->emplace(TAXI_SHOW_DRIVER_INFO, cbTaxiOrderForm);
    handlers->emplace(TAXI_CHANGE_PAYMENT_OR_TARIFF, cbTaxiOrderForm);
    handlers->emplace(TAXI_DISABLED, cbTaxiOrderForm);
    handlers->emplace(TAXI_AFTER_ORDER_ACTIONS, cbTaxiOrderForm);
    handlers->emplace(TAXI_STATUS_ADDRESS, cbTaxiOrderForm);
    handlers->emplace(TAXI_STATUS_PRICE, cbTaxiOrderForm);
    handlers->emplace(TAXI_STATUS_TIME, cbTaxiOrderForm);
    handlers->emplace(TAXI_ORDER_SELECT_CARD, cbTaxiOrderForm);
    handlers->emplace(TAXI_ORDER_CHANGE_CARD, cbTaxiOrderForm);
}

void THandler::AddTaxiSuggest(TContext& ctx, TMaybe<TStringBuf> fromSlotName, TMaybe<TStringBuf> toSlotName) {
    TContext::TSlot* fromSlot = nullptr;
    TContext::TSlot* toSlot = nullptr;
    if (fromSlotName) {
        fromSlot = ctx.GetSlot(*fromSlotName);
    }
    if (fromSlotName) {
        toSlot = ctx.GetSlot(*toSlotName);
    }

    if (ctx.HasExpFlag(EXPERIMENTAL_FLAG_TAXI_ZONEINFO)) {
        TTaxiApi taxiApi{ctx};
        if (!IsSlotEmpty(fromSlot) && !IsTaxiAvailable(taxiApi, fromSlot->Value)
            || !IsSlotEmpty(toSlot) && !IsTaxiAvailable(taxiApi, toSlot->Value)) {
            return;
        }
    } else {
        const auto& geobase = ctx.GlobalCtx().GeobaseLookup();
        NGeobase::TId fromRegion = IsSlotEmpty(fromSlot)
            ? NGeobase::UNKNOWN_REGION
            : GetLocationGeoId(fromSlot->Value);
        NGeobase::TId toRegion = IsSlotEmpty(toSlot)
            ? NGeobase::UNKNOWN_REGION
            : GetLocationGeoId(toSlot->Value);

        if (!IsRussianRegion(geobase, ctx.UserRegion())
            || !IsRussianRegion(geobase, fromRegion)
            || !IsRussianRegion(geobase, toRegion)) {
            return;
        }
    }

    NSc::TValue formUpdate;
    formUpdate["name"] = TAXI_ORDER;

    if (!IsSlotEmpty(fromSlot)) {
        CopySlot(*fromSlot, RESOLVED_LOCATION_FROM, &formUpdate);
    }
    if (!IsSlotEmpty(toSlot)) {
        CopySlot(*toSlot, RESOLVED_LOCATION_TO, &formUpdate);
    }

    ctx.AddSuggest(TStringBuf("taxi_order_fallback"), NSc::Null(), formUpdate);
}

NSc::TValue THandler::GetFormUpdate(const NSc::TValue& locationTo) {
    NSc::TValue formUpdate;
    formUpdate["name"] = TAXI_ORDER;
    TContext::TSlot resolvedLocationSlot(RESOLVED_LOCATION_TO, GEO);
    resolvedLocationSlot.Value = locationTo;
    formUpdate["slots"].SetArray().Push(resolvedLocationSlot.ToJson());
    return formUpdate;
}

TResultValue THandler::ConfirmOrder(TContext& ctx, TTaxiApi& taxiApi, TPersonalDataHelper& personalData, const TPersonalDataHelper::TUserInfo& blackBoxInfo) {

    const TSlot* slotInputResolvedTo = ctx.GetSlot(RESOLVED_LOCATION_TO);
    const TSlot* slotInputResolvedFrom = ctx.GetSlot(RESOLVED_LOCATION_FROM);
    TSlot* confirmSlot = ctx.GetSlot(CONFIRM);
    TSlot* status = ctx.GetOrCreateSlot(STATUS_SLOT_NAME, STRING_TYPE);

    if (!IsSlotEmpty(slotInputResolvedTo) && slotInputResolvedTo->Value.Has(LOCATION)
        && !IsSlotEmpty(slotInputResolvedFrom) && slotInputResolvedFrom->Value.Has(LOCATION)) {
        if (IsSlotEmpty(confirmSlot) || confirmSlot->Value == "no") {
            AddAttention(METRICS_CONFIRMATION_CANCEL, ctx);
            status->Value = ToString(EConfirmOrderStatus::NotConfirmed);
            StopListeningIfSmartSpeaker(ctx);
        } else if (confirmSlot->Value == "yes") {

            // for testing only
            TStringBuilder comment;
            if (ctx.HasExpFlag(EXPERIMENTAL_FLAG_TAXI_TESTING_COMMENTS)) {
                ctx.ClientFeatures().Experiments().OnEachFlag(
                    [&comment](TStringBuf flag) {
                        if (flag.StartsWith(EXPERIMENT_TESTING_COMMENT_PREFIX)) {
                            comment << flag.Tail(EXPERIMENT_TESTING_COMMENT_PREFIX.Size()) << ", ";
                        }
                    }
                );
            }

            TString orderId;
            // flag for testing only
            if (!ctx.HasExpFlag(EXPERIMENTAL_FLAG_TAXI_DO_NOT_MAKE_ORDER)) {
                if (TMakeOrderResult err = taxiApi.MakeOrder(slotInputResolvedFrom->Value, slotInputResolvedTo->Value, comment, &orderId)) {
                    HandleCustomError(ctx, *err, TStringBuf("make_order"), {ORDER_TAXI_SUGGEST, CALL_TO_SUPPORT_SUGGEST}, true /*addSearchAndOnboardingSuggests*/);
                    return TResultValue();
                }
            }
            status->Value = ToString(EConfirmOrderStatus::Ok);
            ctx.AddSuggest(GET_STATUS_SUGGEST);
            ctx.AddSuggest(CANCEL_ORDER_SUGGEST);

            NSc::TValue history;
            auto datasyncError = GetTaxiHistory(personalData, blackBoxInfo, history);
            if (!datasyncError || *datasyncError == TError::EType::NODATASYNCKEYFOUND) {
                bool somethingChanged = false;
                if (!history[USER_ALREADY_MADE_ORDERS_IN_ANY_APP].GetBool()) {
                    ctx.AddAttention(FIRST_ORDER);
                    history[USER_ALREADY_MADE_ORDERS_IN_ANY_APP].SetBool(true);
                    somethingChanged = true;
                }
                if (!IsDeviceWithBrowser(ctx) && !history[USER_ALREADY_MADE_ORDERS_IN_SMART_SPEAKER].GetBool()) {
                    taxiApi.SendPushes(LOCAL_SEND_LEGAL);
                    history[USER_ALREADY_MADE_ORDERS_IN_SMART_SPEAKER].SetBool(true);
                    somethingChanged = true;
                }

                if (somethingChanged) {
                    SaveTaxiHistory(personalData, blackBoxInfo, history);
                }
            }

            AddAttention(METRICS_SUCCESS_CONFIRM, ctx, true /*count stats*/);
            ctx.GetAnalyticsInfoBuilder().AddAction("taxi.order.confirmed", "order.confirmed", "Заказ такси подтвержден");
            NAlice::NScenarios::TAnalyticsInfo::TObject object;
            object.SetId("taxi.order");
            object.SetName("order.id");
            object.SetHumanReadable("Номер заказа");
            object.MutableTaxiOrder()->SetId(orderId);
            ctx.GetAnalyticsInfoBuilder().AddObject(object);
            StopListeningIfSmartSpeaker(ctx);
        } else if (confirmSlot->Value == WRONG) {
            TContext::TPtr newCtx = HandleWrongAddress(ctx, confirmSlot);
            AddAttention(METRICS_WRONG_ADDRESS, *newCtx, true /*count stats*/);
        }
    } else {
        if (IsSlotEmpty(confirmSlot) || confirmSlot->Value == NO) {
            status->Value = ToString(EConfirmOrderStatus::NotConfirmed);
            AddAttention(METRICS_CONFIRMATION_CANCEL_BEFORE_OFFER, ctx);
        } else {
            bool shouldReaskAddress = (confirmSlot->Value == WRONG);
            TContext::TPtr newCtx = HandleWrongAddress(ctx, confirmSlot, shouldReaskAddress /* reaskAddresses */);
            newCtx->AddAttention(CONFIRM_CONFUSED);
            AddAttention(METRICS_CONFIRMATION_FALSE_POSITIVE, *newCtx);
        }
    }
    return TResultValue();
}

TResultValue THandler::MakeOffer(TContext& ctx, TTaxiApi& taxiApi, TPersonalDataHelper& personalData, const TPersonalDataHelper::TUserInfo& blackBoxInfo) {
    {
        auto statusSlotValue = ctx.GetOrCreateSlot(STATUS_SLOT_NAME, STRING_TYPE)->Value;
        if (statusSlotValue == TAXI_SEARCH_FAILED) {
            ctx.AddAttention(TAXI_SEARCH_FAILED);
        }
    }

    // If user says that we incorrect resolve location we need ask both points.
    // Point FROM already asked at this moment
    {
        auto confirmResponse = ctx.GetOrCreateSlot(CONFIRMATION_WRONG, STRING_TYPE);
        if (!IsSlotEmpty(confirmResponse)) {
            if (!IsSlotEmpty(ctx.GetOrCreateSlot(WHERE_FROM, STRING_TYPE)) ||
                !IsSlotEmpty(ctx.GetOrCreateSlot(WHAT_FROM, STRING_TYPE))) {
                confirmResponse->Reset();
                AskWhereTo(ctx);
            }
            return TResultValue();
        }
    }

    // From ChangePaymentOrTariff. We do not need to store the value of this slot for more than 1 request.
    ctx.GetOrCreateSlot(TStringBuf("option_changed"), STRING_TYPE)->Value.SetBool(false);

    // flag for testing only
    if (!ctx.HasExpFlag(EXPERIMENTAL_FLAG_TAXI_DO_NOT_CHECK_PREVIOUS_ORDER)) {
        // check previous order status
        EOrderStatus status;
        NSc::TValue orderData;

        if (TGetStatusResult err = taxiApi.GetStatus(&status, &orderData)) {
            if (*err != EGetStatusError::NoOrdersFound) {
                HandleCustomError(ctx, *err, TStringBuf("check_previous_order"), {GET_STATUS_SUGGEST, CALL_TO_SUPPORT_SUGGEST, CALL_TO_DRIVER_SUGGEST}, true /*addSearchAndOnboardingSuggests*/);
                return TResultValue();
            }
        } else if (IsActiveOrder(status)) {
            TContext::TPtr newContext = ctx.SetResponseForm(TAXI_STATUS, false /* setCurrentFormAsCallback */);
            newContext->AddAttention(TStringBuf("taxi_order_already_exist"));
            newContext->CopySlotsFrom(ctx, {STATUS_REDIRECT_SLOT_NAME});
            ctx.RunResponseFormHandler();
            AddAttention(METRICS_ORDER_ALREADY_EXISTS, ctx);
            return TResultValue();
        }
    }

    if (!IsSlotEmpty(ctx.GetSlot(STOP_OPTIONS_SLOT))) {
        AddAttention(METRICS_ORDER_STOP_OPTIONS, ctx);
        return GenLinkToSite(ctx);
    }

    // for testing only
    if (ctx.HasExpFlag(EXPERIMENTAL_FLAG_TAXI_CLEAR_HISTORY)) {
        SaveTaxiHistory(personalData, blackBoxInfo, {});
    }

    TRouteResolver routeResolver = TRouteResolver(ctx);
    NSc::TValue locationFrom, anotherFrom;
    NSc::TValue locationTo, anotherTo;
    NSc::TValue locationUnknown, anotherUnknown;

    // work with taxi history
    {
        NSc::TValue history;
        auto datasyncHistoryError = GetTaxiHistory(personalData, blackBoxInfo, history);

        if (!IsDeviceWithBrowser(ctx)) {
            if (!datasyncHistoryError && !history[USER_ALREADY_GET_OFFER_IN_SMART_SPEAKER].GetBool()
                || datasyncHistoryError && *datasyncHistoryError == TError::EType::NODATASYNCKEYFOUND)
            {
                ctx.AddAttention(FIRST_ORDER);
                history[USER_ALREADY_GET_OFFER_IN_SMART_SPEAKER].SetBool(true);
            }
        }

        // Resolve location_from
        {
            TResultValue datasyncSmartDeviceGeoError{};
            NSc::TValue smartDeviceGeo;
            if (ctx.MetaClientInfo().IsSmartSpeaker()) {
                const auto* confirmationSlot = ctx.GetSlot(CONNECT_NAMED_LOCATION_TO_DEVICE_CONFIRMATION);
                if (!IsSlotEmpty(confirmationSlot)) {
                    smartDeviceGeo[LOCATION] = confirmationSlot->Value.GetString() == "yes" ? DEFAULT_LOCATION.AsString() : "";
                } else {
                    datasyncSmartDeviceGeoError = GetSmartDeviceGeoData(personalData, blackBoxInfo, smartDeviceGeo);
                }
            }

            bool resolveFromRequest = false;
            if (IsSlotEmpty(ctx.GetOrCreateSlot(WHAT_FROM, STRING_TYPE)) ||
                IsSlotEmpty(ctx.GetOrCreateSlot(WHERE_FROM, STRING_TYPE)))
            {
                resolveFromRequest = true;
            }

            if (const TResolveLocationFromResult err = ResolveLocationFrom(ctx, routeResolver, locationFrom,
                    anotherFrom, datasyncSmartDeviceGeoError, smartDeviceGeo, history))
            {
                AddAttention(METRICS_RESOLVE_LOCATION_FROM_ERROR, ctx);
                TContext::TPtr newContext;
                switch (*err) {
                    case EResolveLocationFromError::NeedAskGeoPointId:
                        TSaveAddressHandler::SetAsResponse(ctx, DEFAULT_LOCATION, TSaveAddressHandler::EModes::ConnectDeviceToAddress);
                        ctx.RunResponseFormHandler();
                        return TResultValue();
                    case EResolveLocationFromError::BadAccuracy:
                        AddAttention(METRICS_BAD_USER_LOCATION_ACCURACY, ctx, true /* count stats */);
                    case EResolveLocationFromError::GeoNotFound:
                        // error block already added to ctx in bass_route_tools
                        AskWhereFrom(ctx);
                        return TResultValue();
                    default:
                        return TError(TError::EType::TAXIERROR, "Geo unexpected error");
                }
            }

            // if we determine correct coordinates by users geoposition incorrect address is not dangerous
            // if we get not exact location in smartspeaker, that saved as "home" then there's nothing we can do
            // so check only if user specify location manually
            if (resolveFromRequest && !IsExactGeo(locationFrom)) {
                if (!IsAlmostExactGeo(locationFrom)) {
                    ctx.AddAttention(FROM_INCOMPLETE);
                    AskWhereFrom(ctx);
                    return TResultValue();
                } else {
                    ctx.AddAttention(FROM_ALMOST_EXACT);
                }
            }

            // resolve location_unknown if it is not empty
            TSlot* checkWhereUnknown = GetFirstNonEmptySlot(ctx, UNKNOWN_SLOT_NAMES);
            if (!IsSlotEmpty(checkWhereUnknown)) {
                const TResolveLocationToResult err = ResolveLocationUnknown(ctx, routeResolver, locationFrom, locationUnknown, anotherUnknown);
                if (!err && IsAlmostExactGeo(locationUnknown)) {
                    double distance = CalcEarthDistance(locationFrom["location"], locationUnknown["location"]);
                    if (distance > UNKNOWN_IS_FROM_THRESHOLD) {
                        // In theory, the tagger should not fill these slots if _unknown is filled
                        // but anything can happen.
                        if (IsSlotEmpty(ctx.GetOrCreateSlot(WHAT_TO, STRING_TYPE)) &&
                            IsSlotEmpty(ctx.GetOrCreateSlot(WHERE_TO, STRING_TYPE)))
                        {
                            // exact or not will be checked later in block of resolving locationTo
                            ctx.GetOrCreateSlot(RESOLVED_LOCATION_TO, GEO_TYPE)->Value = locationUnknown;
                        }
                    } else {
                        // if we determine exact address, but user say not exact
                        if (!IsExactGeo(locationUnknown) && IsExactGeo(locationFrom))
                        {
                            auto& locationFromAddress = GetLocationAddress(locationFrom);
                            auto& locationUnknownAddress = GetLocationAddress(locationUnknown);

                            if (!locationFromAddress[STREET].IsNull() || locationFromAddress[STREET] != locationUnknownAddress[STREET]) {
                                ctx.GetOrCreateSlot(RESOLVED_LOCATION_FROM, GEO_TYPE)->Value = locationUnknown;
                            }
                        } else {
                            if (!IsExactGeo(locationFrom) && IsExactGeo(locationUnknown)) {
                                ctx.DeleteAttention(FROM_ALMOST_EXACT);
                            }

                            ctx.GetOrCreateSlot(RESOLVED_LOCATION_FROM, GEO_TYPE)->Value = locationUnknown;
                        }
                    }
                }
            }
        }

        // check if location supported by Yandex Taxi
        if (const TGetLocationInfoResult err = taxiApi.GetLocationInfo(locationFrom)) {
            if (*err == ELocationInfoError::LocationIsNotSupported) {
                AddAttention(METRICS_ZONEINFO_LOCATION_IS_NOT_SUPPORTED, ctx);
                ctx.AddAttention(LOCATION_IS_NOT_SUPPORTED);
                AskWhereFrom(ctx);
            } else {
                HandleCustomError(ctx, *err, TStringBuf("zone_info"), {CALL_TO_SUPPORT_SUGGEST}, true /*addSearchAndOnboardingSuggests*/);
            }
            return TResultValue();
        }

        // select tariff
        if (ctx.GetOrCreateSlot(CHECKED_TARIFF, STRING_TYPE)->Value.IsNull()) {
            TStringBuf tariff = ctx.GetOrCreateSlot(TARIFF_SLOT, STRING_TYPE)->Value.GetString();
            tariff = tariff ? tariff : history[PREVIOUS_TARIFF].GetString();
            if (TSetTariffResult err = taxiApi.SetTariff(tariff)) {
                if (err == ESetTariffError::UnsupportedTariff) {
                    TContext::TPtr newContext = ctx.SetResponseForm(TAXI_CHANGE_PAYMENT_OR_TARIFF, false /* setCurrentFormAsCallback */);
                    CopyImportantSlotsValues(ctx, *newContext);

                    NSc::TValue availableTariffs;
                    taxiApi.GetTariffsList(availableTariffs);

                    newContext->AddAttention(TARIFF_NOT_AVAILABLE);
                    newContext->GetOrCreateSlot(AVAILABLE_TARIFFS, ARRAY_TYPE)->Value = availableTariffs;
                    newContext->GetOrCreateSlot(WHAT_CHANGE, ARRAY_TYPE)->Value.SetString(TARIFF);
                    // No need to run Response Form Handler
                    return TResultValue();
                } else {
                    HandleCustomError(ctx, *err, TStringBuf("SetTariff"), {ORDER_TAXI_SUGGEST, CALL_TO_SUPPORT_SUGGEST});
                    return TResultValue();
                }
            }
            history[PREVIOUS_TARIFF].SetString(tariff);
        }

        // select payment method
        NSc::TValue& newSelectedCard = ctx.GetOrCreateSlot(SELECTED_CARD, STRING_TYPE)->Value;
        if (!newSelectedCard.IsNull()) {
            history["last_card"].CopyFrom(newSelectedCard);
        }

        if (ctx.GetOrCreateSlot(CHECKED_PAYMENT_METHOD, STRING_TYPE)->Value.IsNull()) {
            TStringBuf method = ctx.GetOrCreateSlot(PAYMENT_METHOD_SLOT, STRING_TYPE)->Value.GetString();

            if (TSetPaymentMethodResult err = taxiApi.SetPaymentMethod(method, locationFrom, history)) {
                if (err == ESetPaymentMethodError::PaymentMethodNotAvailable) {
                    TContext::TPtr newContext = ctx.SetResponseForm(TAXI_CHANGE_PAYMENT_OR_TARIFF, false /* setCurrentFormAsCallback */);
                    CopyImportantSlotsValues(ctx, *newContext);
                    newContext->AddAttention(PAYMENT_METHOD_NOT_AVAILABLE);
                    newContext->GetOrCreateSlot(WHAT_CHANGE, ARRAY_TYPE)->Value.SetString(PAYMENT);
                    ctx.RunResponseFormHandler();
                    return TResultValue();
                } else {
                    HandleCustomError(ctx, *err, TStringBuf("set_payment_method"), {ORDER_TAXI_SUGGEST, CALL_TO_SUPPORT_SUGGEST});
                    return TResultValue();
                }
            }
        }

        if (!datasyncHistoryError || *datasyncHistoryError == TError::EType::NODATASYNCKEYFOUND) {
            SaveTaxiHistory(personalData, blackBoxInfo, history);
        }
    }

    // we need to know destination
    TSlot* checkWhereTo = GetFirstNonEmptySlot(ctx, TO_SLOT_NAMES);
    if (!checkWhereTo) {
        AskWhereTo(ctx);
        return TResultValue();
    }

    // Resolve location_to
    {
        if (const TResolveLocationToResult err = ResolveLocationTo(ctx, routeResolver, locationFrom, locationTo, anotherTo)) {
            // error block already added to ctx in bass_route_tools
            AddAttention(METRICS_RESOLVE_LOCATION_TO_ERROR, ctx);
            switch (*err) {
                case EResolveLocationToError::GeoNotFound:
                    AskWhereTo(ctx);
                    return TResultValue();
                default:
                    return TError(TError::EType::TAXIERROR, "Geo unexpected error");
            }
        }

        if (!IsExactGeo(locationTo)) {
            if (!IsAlmostExactGeo(locationTo)) {
                ctx.AddAttention(TO_INCOMPLETE);
                AskWhereTo(ctx);
                return TResultValue();
            } else {
                ctx.AddAttention(TO_ALMOST_EXACT);
            }
        }
    }

    // Get price and offer
    NSc::TValue result;
    if (TGetPriceResult err = taxiApi.GetPrice(locationFrom, locationTo, &result)) {
        if (*err == EGetPriceError::LocationIsNotSupported) {
            AddAttention(METRICS_ESTIMATE_LOCATION_IS_NOT_SUPPORTED, ctx);
            ctx.AddAttention(LOCATION_IS_NOT_SUPPORTED);
            AskWhereFrom(ctx);
        } else {
            HandleCustomError(ctx, *err, TStringBuf("estimate"), {ORDER_TAXI_SUGGEST, CALL_TO_SUPPORT_SUGGEST});
        }
        return TResultValue();
    }

    // fill price
    {
        auto priceSlot = ctx.GetOrCreateSlot("price", STRING_TYPE);
        if (result["is_fixed_price"].GetBool()) {
            priceSlot->Value = result["price"].GetString();
        } else {
            priceSlot->Value = "";
        }
    }

    ctx.GetOrCreateSlot(OFFER, STRING_TYPE)->Value = result[OFFER].GetString();
    ctx.GetOrCreateSlot(ESTIMATE_ROUTE_TIME_MINUTES, STRING_TYPE)->Value = result[ESTIMATE_ROUTE_TIME_MINUTES].ForceString();
    ctx.GetOrCreateSlot(WAITING_TIME_MINUTES, STRING_TYPE)->Value = result[WAITING_TIME_MINUTES].ForceString();
    ctx.GetOrCreateSlot(STATUS_SLOT_NAME, STRING_TYPE)->Value = "ok";

    ctx.AddSuggest(OK_SUGGEST);
    ctx.AddSuggest(CHANGE_ADDRESS_SUGGEST);
    ctx.AddSuggest(CHANGE_PAYMENT_SUGGEST);
    ctx.AddSuggest(CHANGE_TARIFF_SUGGEST);
    ctx.AddSuggest(CANCEL_SUGGEST);
    AddBrowserSuggest(ctx, SHOW_LEGAL_SUGGEST);
    AddAttention(METRICS_OFFER_SUCCESS, ctx);

    // for testing only
    if (ctx.HasExpFlag(EXPERIMENTAL_FLAG_TAXI_CLEAR_DEVICE_GEOPOINTS)) {
        SaveSmartDeviceGeoData(personalData, blackBoxInfo, {});
    }

    return TResultValue();
}

TResultValue THandler::ShowLegal(TContext& ctx, NTaxi::TTaxiApi& taxiApi) {
    if (IsDeviceWithBrowser(ctx)) {
        NSc::TValue taxiOrderData;
        taxiOrderData["uri"].SetString(LEGAL_LINK);
        ctx.AddCommand<TTaxiShowLegalDirective>(OPEN_URI, taxiOrderData);
        ctx.AddSuggest(SHOW_LEGAL_COMMAND, taxiOrderData);

        ctx.AddSuggest(GET_STATUS_SUGGEST);

        AddAttention(METRICS_SHOW_LEGAL_SUCCESS, ctx);
    } else {
        ctx.AddAttention(NEED_APPLICATION_DEVICE);
        taxiApi.SendPushes(LOCAL_SEND_LEGAL);
        AddAttention(METRICS_SHOW_LEGAL_TRY_FROM_NOT_PHONE, ctx);
        ctx.AddStopListeningBlock();
    }
    return TResultValue();
}

TResultValue THandler::OpenApp(TContext& ctx) {
    if (IsDeviceWithBrowser(ctx)) {
        const TSlot* slotInputResolvedTo = ctx.GetOrCreateSlot(RESOLVED_LOCATION_TO, GEO_TYPE);
        const TSlot* slotInputResolvedFrom = ctx.GetOrCreateSlot(RESOLVED_LOCATION_FROM, GEO_TYPE);
        TString taxiUri = GenerateTaxiUri(ctx, slotInputResolvedFrom->Value, slotInputResolvedTo->Value, false /*use fallback to taxi site*/);
        // add command block and suggest
        NSc::TValue taxiOrderData;
        taxiOrderData["uri"].SetString(taxiUri);
        ctx.AddCommand<TTaxiOpenAppWithOrderDirective>(OPEN_URI, taxiOrderData);
        ctx.AddSuggest(OPEN_APP_COMMAND, taxiOrderData);
        ctx.GetOrCreateSlot(STATUS_SLOT_NAME, STRING_TYPE)->Value.SetString(TStringBuf("OpenApp"));
        AddAttention(METRICS_OPEN_APP_SUCCESS, ctx);
    } else {
        ctx.AddAttention(NEED_APPLICATION_DEVICE);
        AddAttention(METRICS_OPEN_APP_TRY_FROM_NOT_PHONE, ctx);
        ctx.AddStopListeningBlock();
    }
    ctx.AddSuggest(GET_STATUS_SUGGEST);
    ctx.AddSuggest(CANCEL_ORDER_SUGGEST);
    ctx.AddSuggest(WHO_IS_TRANSPORTER_SUGGEST);
    return TResultValue();
}

TResultValue THandler::CallToSupport(TContext& ctx, NTaxi::TTaxiApi& taxiApi) {
    if (auto err = taxiApi.SendMessageToSupport()) {
        HandleCustomError(ctx, *err, TStringBuf("send_support_ticket"), {CALL_TO_SUPPORT_SUGGEST}, true /*addSearchAndOnboardingSuggests*/);
        return TResultValue();
    }
    if (!IsPhoneDevice(ctx)) {
        AddAttention(METRICS_CALL_TO_SUPPORT_TRY_FROM_NOT_PHONE, ctx);
        ctx.AddStopListeningBlock();
    }
    ctx.AddAttention(TStringBuf("ticket_created"));
    AddAttention(METRICS_CALL_TO_SUPPORT_SUCCESS, ctx);
    ctx.AddSuggest(GET_STATUS_SUGGEST);
    ctx.AddSuggest(CANCEL_ORDER_SUGGEST);
    ctx.AddSuggest(WHO_IS_TRANSPORTER_SUGGEST);
    return TResultValue();
}

TResultValue THandler::CallToDriver(TContext& ctx, TTaxiApi& taxiApi) {
    if (IsPhoneDevice(ctx)) {
        EOrderStatus status;
        NSc::TValue orderData;

        if (TGetStatusResult err = taxiApi.GetStatus(&status, &orderData)) {
            HandleCustomError(ctx, *err, TStringBuf("check_current_status"), {GET_STATUS_SUGGEST, CALL_TO_SUPPORT_SUGGEST, CALL_TO_DRIVER_SUGGEST}, true /*addSearchAndOnboardingSuggests*/);
            return TResultValue();
        }
        TString driversPhone = TString(orderData["drivers_phone"].GetString());
        if (IsActiveOrder(status) && driversPhone) {
            TString phoneUri = GeneratePhoneUri(ctx.MetaClientInfo(), driversPhone, false /* normalize */);

            // add command block and suggest
            NSc::TValue phoneData;
            phoneData["uri"].SetString(phoneUri);
            ctx.AddCommand<TTaxiCall2DriverDirective>(OPEN_URI, phoneData);
            ctx.AddSuggest(CALL_TO_DRIVER_COMMAND, phoneData);
            ctx.AddSuggest(GET_STATUS_SUGGEST);
            AddPhoneSuggest(ctx, CALL_TO_SUPPORT_SUGGEST);
            ctx.AddSuggest(CANCEL_ORDER_SUGGEST);
            ctx.AddSuggest(WHO_IS_TRANSPORTER_SUGGEST);
            AddAttention(METRICS_CALL_TO_DRIVER_SUCCESS, ctx);
        } else if (IsActiveOrder(status) && !driversPhone) {
            if (IsKnownDriver(status)) {
                ctx.AddAttention(NO_PHONE_NUMBER);
            } else {
                ctx.AddAttention(NO_DRIVER_YET);
            }
            ctx.AddSuggest(GET_STATUS_SUGGEST);
            AddPhoneSuggest(ctx, CALL_TO_SUPPORT_SUGGEST);
            ctx.AddSuggest(CANCEL_ORDER_SUGGEST);
            ctx.AddSuggest(WHO_IS_TRANSPORTER_SUGGEST);
            AddAttention(METRICS_CALL_TO_DRIVER_NO_DRIVERS_PHONE, ctx, true /*count stats*/);
        } else {
            ctx.AddAttention(NO_ACTIVE_ORDERS);
            ctx.AddSuggest(ORDER_TAXI_SUGGEST);
            AddPhoneSuggest(ctx, CALL_TO_SUPPORT_SUGGEST);
            AddAttention(METRICS_CALL_TO_DRIVER_NO_ORDER, ctx);
        }
    } else {
        ctx.AddAttention(NEED_PHONE_DEVICE);
        AddAttention(METRICS_CALL_TO_DRIVER_TRY_FROM_NOT_PHONE, ctx);
        taxiApi.SendPushes(LOCAL_CALL_TO_DRIVER);
    }

    StopListeningIfSmartSpeaker(ctx);
    return TResultValue();
}

TResultValue THandler::GenLinkToSite(TContext& ctx) {
    // First of all we need to know destination
    TSlot* checkWhereTo = GetFirstNonEmptySlot(ctx, TO_SLOT_NAMES);
    if (!checkWhereTo) {
        AskWhereTo(ctx);
        return TResultValue();
    }

    TRouteResolver routeResolver = TRouteResolver(ctx);

    // Resolve location_from
    NSc::TValue locationFrom, anotherFrom;
    if (const TResolveLocationFromResult err = ResolveLocationFromOld(ctx, routeResolver, locationFrom, anotherFrom)) {
        // error block already added to ctx
        AddAttention(METRICS_RESOLVE_LOCATION_FROM_ERROR, ctx);
        switch (*err) {
            case EResolveLocationFromError::GeoNotFound:
                AskWhereFrom(ctx);
                return TResultValue();
            default:
                return TError(TError::EType::TAXIERROR, "Geo unexpected error");
        }
    }

    // Resolve location_to
    NSc::TValue locationTo, anotherTo;
    if (const TResolveLocationToResult err = ResolveLocationTo(ctx, routeResolver, locationFrom, locationTo, anotherTo)) {
        // error block already added to ctx
        AddAttention(METRICS_RESOLVE_LOCATION_TO_ERROR, ctx);
        switch (*err) {
            case EResolveLocationToError::GeoNotFound:
                AskWhereTo(ctx);
                return TResultValue();
            default:
                return TError(TError::EType::TAXIERROR, "Geo unexpected error");
        }
    }

    // we do not use user location as start point for taxi order
    if (IsSlotEmpty(ctx.GetSlot(WHAT_FROM)) && TSpecialLocation::IsNearLocation(ctx.GetSlot(WHERE_FROM))) {
        locationFrom.SetNull();
    }

    TString taxiUri = GenerateTaxiUri(ctx, locationFrom, locationTo);

    // add command block and suggest
    NSc::TValue taxiOrderData;
    taxiOrderData["uri"].SetString(taxiUri);
    ctx.AddCommand<TTaxiOpenAppWithOrderDirective>(OPEN_URI, taxiOrderData);
    ctx.AddSuggest(OPEN_APP_COMMAND, taxiOrderData);

    ctx.GetOrCreateSlot(DEEP_LINK, STRING_TYPE)->Value = taxiUri;

    AddAttention(METRICS_LINK_TO_SITE_SUCCESS, ctx);
    return TResultValue();
}

TResultValue THandler::GetOrderStatus(TContext& ctx, TTaxiApi& taxiApi, TPersonalDataHelper& personalData, const TPersonalDataHelper::TUserInfo& blackBoxInfo) {
    EOrderStatus status;
    NSc::TValue orderData;

    if (TGetStatusResult err = taxiApi.GetStatus(&status, &orderData)) {
        HandleCustomError(ctx, *err, TStringBuf("check_current_status"), {CALL_TO_DRIVER_SUGGEST, GET_STATUS_SUGGEST, CALL_TO_SUPPORT_SUGGEST}, true /*addSearchAndOnboardingSuggests*/);
        return TResultValue();
    }

    FillStatusForm(ctx, status, orderData);

    switch (status) {
        case EOrderStatus::Scheduling:
        case EOrderStatus::Scheduled:
        case EOrderStatus::Searching:
            if (!ctx.HasExpFlag(EXPERIMENTAL_FLAG_TAXI_DO_NOT_ADD_OPEN_IN_APP_SUGGEST)) {
                AddBrowserSuggest(ctx, OPEN_IN_APP_SUGGEST);
            }
            AddPhoneSuggest(ctx, CALL_TO_SUPPORT_SUGGEST);
            ctx.AddSuggest(CANCEL_ORDER_SUGGEST);
            ctx.AddSuggest(GET_STATUS_SUGGEST);
            ctx.AddAttention(NO_DRIVER_YET);
            break;
        case EOrderStatus::Driving:
            if (!IsDeviceWithBrowser(ctx)) {
                NSc::TValue history;
                auto err = GetTaxiHistory(personalData, blackBoxInfo, history);
                if (!err && !history[USER_HAD_DRIVERS_FOR_ORDER_FROM_SMART_SPEAKER].GetBool() || err && *err == TError::EType::NODATASYNCKEYFOUND) {
                    taxiApi.SendPushes(LOCAL_WHO_IS_TRANSPORTER);
                    ctx.AddAttention(FIRST_ORDER);
                    history[USER_HAD_DRIVERS_FOR_ORDER_FROM_SMART_SPEAKER].SetBool(true);
                    SaveTaxiHistory(personalData, blackBoxInfo, history);
                }
            }
        case EOrderStatus::Waiting:
        case EOrderStatus::Transporting:
            if (!ctx.HasExpFlag(EXPERIMENTAL_FLAG_TAXI_DO_NOT_ADD_OPEN_IN_APP_SUGGEST)) {
                AddBrowserSuggest(ctx, OPEN_IN_APP_SUGGEST);
            }
            AddPhoneSuggest(ctx, CALL_TO_DRIVER_SUGGEST);
            ctx.AddSuggest(WHO_IS_TRANSPORTER_SUGGEST);
            AddPhoneSuggest(ctx, CALL_TO_SUPPORT_SUGGEST);
            ctx.AddSuggest(CANCEL_ORDER_SUGGEST);
            ctx.AddSuggest(GET_STATUS_SUGGEST);
            if ((ctx.FormName() == TAXI_STATUS || ctx.FormName() == TAXI_STATUS_ADDRESS ) && ctx.ClientFeatures().SupportsDivCards()) {

                NMapsStaticApi::TImageUrlBuilder mapBuilder{ctx};
                mapBuilder.SetSize(MAP_IMAGE_WIDTH, MAP_IMAGE_HEIGHT).Set(TStringBuf("scale"), "1.1");
                double driver_lon = orderData["driver_lon"].ForceNumber();
                double driver_lat = orderData["driver_lat"].ForceNumber();

                mapBuilder.AddPoint(driver_lon, driver_lat, "taxi_big");
                if (status == EOrderStatus::Transporting)
                    mapBuilder.AddPoint(orderData["location_to"]["lon"].ForceNumber(), orderData["location_to"]["lat"].ForceNumber(), "pm2bm");
                else {
                    mapBuilder.AddPoint(orderData["location_from"]["lon"].ForceNumber(),
                                        orderData["location_from"]["lat"].ForceNumber(), "pm2am");
                    if (status == EOrderStatus::Waiting) {
                        // By default map often has too large scale here because of points is too close
                        mapBuilder.SetCenter(driver_lon, driver_lat);
                        mapBuilder.SetZoom(MAP_WAITING_SCALE);
                    }
                }

                ctx.GetOrCreateSlot(MAP_URL_SLOT, STRING_TYPE)->Value = mapBuilder.Build();
                ctx.AddDivCardBlock(TStringBuf("taxi_status_map"), {});
                ctx.AddTextCardBlock(TStringBuf("taxi_status_map_text"), {});
            }
            break;
        case EOrderStatus::Complete:
        case EOrderStatus::Failed:
        case EOrderStatus::Cancelled:
        case EOrderStatus::Expired:
        case EOrderStatus::Preexpired:
        case EOrderStatus::Draft:
        case EOrderStatus::Finished:
        case EOrderStatus::Unknown:
            ctx.AddSuggest(ORDER_TAXI_SUGGEST);
            AddPhoneSuggest(ctx, CALL_TO_SUPPORT_SUGGEST);
            ctx.AddAttention(NO_ACTIVE_ORDERS);
            break;
    }

    StopListeningIfSmartSpeaker(ctx);
    AddAttention(METRICS_STATUS_GET, ctx);
    return TResultValue();
}

TResultValue THandler::GetCancelOffer(TContext& ctx, TTaxiApi& taxiApi) {
    EOrderStatus status;
    NSc::TValue orderData;

    if (TGetStatusResult err = taxiApi.GetStatus(&status, &orderData)) {
        HandleCustomError(ctx, *err, TStringBuf("check_current_status"), {CALL_TO_DRIVER_SUGGEST, GET_STATUS_SUGGEST, CALL_TO_SUPPORT_SUGGEST});
        return TResultValue();
    }

    bool isActiveOrder = IsActiveOrder(status);
    bool isCancelDisabled = orderData[CANCEL_DISABLED].GetBool();
    if (!isActiveOrder || isCancelDisabled) {
        FailToCancelOrder(ctx, status, orderData);
    } else {
        AskForCancelConfirmation(ctx, status, orderData);
    }
    return TResultValue();
}

TResultValue THandler::CancelOrderIfExists(TContext& ctx, TTaxiApi& taxiApi, TPersonalDataHelper& personalData, const TPersonalDataHelper::TUserInfo& blackBoxInfo) {
    EOrderStatus status;
    NSc::TValue orderData;

    bool isActiveOrder = false;
    bool isCancelDisabled = false;
    if (TGetStatusResult err = taxiApi.GetStatus(&status, &orderData)) {
        // if there are no orders, we catch an error, but we shouldn't propagate it, just keep isActiveOrder=false
        if (*err != EGetStatusError::NoOrdersFound) {
            HandleCustomError(ctx, *err, TStringBuf("check_current_status"), {CALL_TO_DRIVER_SUGGEST, GET_STATUS_SUGGEST, CALL_TO_SUPPORT_SUGGEST});
            return TResultValue();
        }
    } else {
        isActiveOrder = IsActiveOrder(status);
        isCancelDisabled = orderData[CANCEL_DISABLED].GetBool();
    }

    if (!isActiveOrder) {
        ctx.GetOrCreateSlot(CONFIRM, STRING_TYPE)->Value.SetString(NO);
        AddAttention(METRICS_ORDER_CONFIRM, ctx);
        return ConfirmOrder(ctx, taxiApi, personalData, blackBoxInfo);
    }
    // if there is an active order, we not just do-not-create order, but cancel an existing one
    TContext::TPtr newContext = ctx.SetResponseForm(TAXI_CANCEL, false /* setCurrentFormAsCallback */);
    if (isCancelDisabled) {
        return FailToCancelOrder(*newContext, status, orderData);
    } else {
        return AskForCancelConfirmation(*newContext, status, orderData);
    }
}

TResultValue THandler::FailToCancelOrder(TContext& ctx, EOrderStatus& status, NSc::TValue& orderData) {
    bool isActiveOrder = IsActiveOrder(status);
    bool isCancelDisabled = orderData[CANCEL_DISABLED].GetBool();
    TContext::TPtr newContext = ctx.SetResponseForm(TAXI_DISABLED, false /* setCurrentFormAsCallback */);
    FillStatusForm(*newContext, status, orderData);
    newContext->GetOrCreateSlot(IS_ACTIVE_ORDER, BOOL_TYPE)->Value.SetBool(isActiveOrder);
    newContext->GetOrCreateSlot(CANCEL_DISABLED, BOOL_TYPE)->Value.SetBool(isCancelDisabled);
    AddPhoneSuggest(*newContext, CALL_TO_SUPPORT_SUGGEST);
    newContext->AddSuggest(GET_STATUS_SUGGEST);
    if (isCancelDisabled) {
        AddAttention(METRICS_CANCEL_DISABLED, *newContext);
    } else if (!isActiveOrder) {
        AddAttention(METRICS_CANCEL_NO_ACTIVE_ORDER, *newContext);
    }
    return TResultValue();
}

TResultValue THandler::AskForCancelConfirmation(TContext& ctx, EOrderStatus& status, NSc::TValue& orderData) {
    if (!orderData["cancel_state"].IsNull() && orderData["cancel_state"] != TStringBuf("free")) {
        ctx.GetOrCreateSlot(CANCEL_MESSAGE, STRING_TYPE)->Value.SetString(orderData[CANCEL_MESSAGE].GetString());
    }
    ctx.AddSuggest(YES_SUGGEST);
    ctx.AddSuggest(NO_SUGGEST);
    ctx.AddSuggest(GET_STATUS_SUGGEST);
    AddPhoneSuggest(ctx, CALL_TO_SUPPORT_SUGGEST);
    FillStatusForm(ctx, status, orderData);
    AddAttention(METRICS_GET_CANCEL_OFFER, ctx);
    return TResultValue();
}

TResultValue THandler::CancelOrder(TContext& ctx, TTaxiApi& taxiApi) {
    TStringBuf orderId = ctx.GetOrCreateSlot(ORDER_ID, STRING_TYPE)->Value.GetString();
    TStringBuf confirm = ctx.GetOrCreateSlot(CONFIRM, STRING_TYPE)->Value.GetString();
    if (confirm == "yes") {
        if (TCancelResult err = taxiApi.CancelOrder(orderId)) {
            if (*err == ECancelError::CanNotCancel) {
                AddPhoneSuggest(ctx, CALL_TO_SUPPORT_SUGGEST);
            }
            HandleCustomError(ctx, *err, TStringBuf("cancel_order"), {CALL_TO_DRIVER_SUGGEST, GET_STATUS_SUGGEST, CALL_TO_SUPPORT_SUGGEST});
            return TResultValue();
        }
        ctx.GetOrCreateSlot(STATUS_SLOT_NAME, STRING_TYPE)->Value = ToString(ECancelOrderStatus::Ok);
        ctx.AddSuggest(ORDER_TAXI_SUGGEST);
        AddAttention(METRICS_CANCEL_CONFIRMED, ctx, true /*count stats*/);
    } else if (confirm == "no") {
        ctx.GetOrCreateSlot(STATUS_SLOT_NAME, STRING_TYPE)->Value = ToString(ECancelOrderStatus::NotConfirmed);
        ctx.AddSuggest(GET_STATUS_SUGGEST);
        AddPhoneSuggest(ctx, CALL_TO_SUPPORT_SUGGEST);
        ctx.AddSuggest(CANCEL_ORDER_SUGGEST);
        ctx.AddSuggest(WHO_IS_TRANSPORTER_SUGGEST);
        AddAttention(METRICS_CANCEL_NOT_CONFIRMED, ctx);
    }
    StopListeningIfSmartSpeaker(ctx);
    return TResultValue();
}

TResultValue THandler::ChangePaymentOrTariff(TContext& ctx, NTaxi::TTaxiApi& taxiApi) {
    TSlot* whatSlot = ctx.GetOrCreateSlot(WHAT_CHANGE, STRING_TYPE);
    TSlot* newPaymentMethodSlot = ctx.GetOrCreateSlot(TStringBuf("new_payment_method"), STRING_TYPE);
    TSlot* newTariffSlot = ctx.GetOrCreateSlot(TStringBuf("new_tariff"), STRING_TYPE);

    if (!IsSlotEmpty(whatSlot) && IsSlotEmpty(newPaymentMethodSlot) && IsSlotEmpty(newTariffSlot)) {
        NSc::TValue locationFrom;
        TSlot* slotInputResolvedFrom = ctx.GetOrCreateSlot(RESOLVED_LOCATION_FROM, STRING_TYPE);
        if (!IsSlotEmpty(slotInputResolvedFrom) && slotInputResolvedFrom->Value.Has(LOCATION)) {
            locationFrom = slotInputResolvedFrom->Value;
        } else {
            ctx.AddAttention(TStringBuf("unknown_location_from"));
            return TResultValue();
        }

        if (whatSlot->Value.GetString() == PAYMENT) {
            NSc::TValue methods;
            if (TGetPaymentMethodsResult err = taxiApi.GetPaymentMethods(methods, locationFrom)) {
                if (err == EGetPaymentMethodsError::NoAvailablePaymentMethods) {
                    ctx.AddAttention(TStringBuf("no_payment_methods_available"));
                } else {
                    HandleCustomError(ctx, *err, TStringBuf("get_payment_methods"), {ORDER_TAXI_SUGGEST, CALL_TO_SUPPORT_SUGGEST});
                }
                return TResultValue();
            }
            NSc::TValue& paymentMethodsSlot = ctx.GetOrCreateSlot(AVAILABLE_PAYMENT_METHODS, STRING_TYPE)->Value;
            for (auto& method : methods.GetArray()) {
                paymentMethodsSlot[method["type"].GetString()].SetBool(true);
            }
        } else if (whatSlot->Value.GetString() == TARIFF) {
            NSc::TValue availableTariffs;
            if (TGetLocationInfoResult err = taxiApi.GetLocationInfo(locationFrom)) {
                HandleCustomError(ctx, *err, TStringBuf("get_location_info"), {ORDER_TAXI_SUGGEST, CALL_TO_SUPPORT_SUGGEST});
                return TResultValue();
            }
            taxiApi.GetTariffsList(availableTariffs);
            ctx.GetOrCreateSlot(AVAILABLE_TARIFFS, ARRAY_TYPE)->Value = availableTariffs;
        } else {
            ctx.AddAttention(NOT_UNDERSTOOD);
        }
    } else {
        TContext::TPtr newContext = ctx.SetResponseForm(TAXI_ORDER_SPECIFY, false /* setCurrentFormAsCallback */);
        CopyImportantSlotsValues(ctx, *newContext);
        TSlot* checkedPaymentMethodSlot = newContext->GetOrCreateSlot(CHECKED_PAYMENT_METHOD, STRING_TYPE);
        TSlot* checkedTariff = newContext->GetOrCreateSlot(CHECKED_TARIFF, STRING_TYPE);

        if (!IsSlotEmpty(newPaymentMethodSlot) || !IsSlotEmpty(newTariffSlot)) {
            newContext->GetOrCreateSlot(PAYMENT_METHOD_SLOT, STRING_TYPE)->Value.CopyFrom(newPaymentMethodSlot->Value);
            newContext->GetOrCreateSlot(TARIFF_SLOT, STRING_TYPE)->Value.CopyFrom(newTariffSlot->Value);
            checkedPaymentMethodSlot->Value.SetNull();
            checkedTariff->Value.SetNull();
        } else {
            newContext->AddAttention(NOT_UNDERSTOOD);
        }

        ctx.RunResponseFormHandler();
        newContext->GetOrCreateSlot(TStringBuf("option_changed"), STRING_TYPE)->Value.SetBool(true);
    }
    return TResultValue();
}

TResultValue THandler::ChangeCard(TContext& ctx, NTaxi::TTaxiApi& taxiApi) {
    NSc::TValue locationFrom;
    NSc::TValue methods;
    TSlot* slotInputResolvedFrom = ctx.GetOrCreateSlot(RESOLVED_LOCATION_FROM, STRING_TYPE);
    if (!IsSlotEmpty(slotInputResolvedFrom) && slotInputResolvedFrom->Value.Has(LOCATION)) {
        locationFrom = slotInputResolvedFrom->Value;
    } else {
        ctx.AddAttention(TStringBuf("unknown_location_from"));
        return TResultValue();
    }

    if (TGetPaymentMethodsResult err = taxiApi.GetPaymentMethods(methods, locationFrom)) {
        if (err == EGetPaymentMethodsError::NoAvailablePaymentMethods) {
            ctx.AddAttention(TStringBuf("no_payment_methods_available"));
        } else {
            HandleCustomError(ctx, *err, TStringBuf("get_payment_methods"), {ORDER_TAXI_SUGGEST, CALL_TO_SUPPORT_SUGGEST});
        }
        return TResultValue();
    }
    NSc::TValue& allowedCardsSlot = ctx.GetOrCreateSlot(ALLOWED_CARDS, STRING_TYPE)->Value;
    allowedCardsSlot.SetNull();
    for (auto& method : methods.GetArray()) {
        if (method["type"] == PAYMENT_CARD) {
            allowedCardsSlot.Push().CopyFrom(method);
        }
    }
    return TResultValue();
}

TResultValue THandler::SelectCard(TContext& ctx) {
    NSc::TValue& selectedCard = ctx.GetOrCreateSlot(SELECTED_CARD, STRING_TYPE)->Value;
    if (selectedCard.IsNull() || !selectedCard[CARD_NUMBER].GetString()) {
        ctx.AddAttention(TStringBuf("card_not_selected"));
        return TResultValue();
    }

    TContext::TPtr newContext = ctx.SetResponseForm(TAXI_ORDER_SPECIFY, false /* setCurrentFormAsCallback */);
    CopyImportantSlotsValues(ctx, *newContext);
    newContext->GetOrCreateSlot(CHECKED_PAYMENT_METHOD, STRING_TYPE)->Value.SetNull();
    newContext->GetOrCreateSlot(PAYMENT_METHOD_SLOT, STRING_TYPE)->Value.SetString(PAYMENT_CARD);
    newContext->GetOrCreateSlot(SELECTED_CARD, STRING_TYPE)->Value.CopyFrom(selectedCard);
    ctx.RunResponseFormHandler();
    return TResultValue();
}
} // namespace NBass::NTaxi
