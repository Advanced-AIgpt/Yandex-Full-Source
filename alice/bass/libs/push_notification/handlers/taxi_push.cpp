#include "taxi_push.h"
#include "handler.h"

#include <library/cpp/cgiparam/cgiparam.h>

namespace NBASS::NPushNotification {
namespace {
// tags
constexpr TStringBuf TAXI_EVENT_TAG = "taxi_event";
constexpr TStringBuf TAXI_LOCAL_TAG = "taxi_local";
constexpr TStringBuf TAXI_LEGAL_TAG = "taxi_legal";
constexpr TStringBuf TAXI_DRIVER_INFO = "taxi_driver_info";

// titles
constexpr TStringBuf CANT_RESERVE_MONEY = "Не удалось списать деньги";
constexpr TStringBuf DRIVER_ARRIVED = "Такси уже здесь";
constexpr TStringBuf DRIVER_DISCARDS_ORDER = "Водитель отменил заказ";
constexpr TStringBuf DRIVER_FOUND = "Водитель найден";
constexpr TStringBuf DRIVER_IS_NEAR = "Такси подъезжает";
constexpr TStringBuf ORDER_FAILED = "Не удалось найти такси";
constexpr TStringBuf WAITING_TIMEOUT = "Истекло время ожидания";
constexpr TStringBuf ALICE = "Алиса";

constexpr TStringBuf QUASAR_EVENT = "server_action";

// intents
constexpr TStringBuf TAXI_CALL_TO_DRIVER = "personal_assistant.scenarios.taxi_new_call_to_driver";
constexpr TStringBuf TAXI_CALL_TO_SUPPORT = "personal_assistant.scenarios.taxi_new_call_to_support";
constexpr TStringBuf TAXI_STATUS_FORM = "personal_assistant.scenarios.taxi_new_status";
constexpr TStringBuf TAXI_ORDER = "personal_assistant.scenarios.taxi_new_order";
constexpr TStringBuf TAXI_SHOW_LEGAL = "personal_assistant.scenarios.taxi_new_show_legal";
constexpr TStringBuf TAXI_SHOW_DRIVER_INFO = "personal_assistant.scenarios.taxi_new_show_driver_info";

constexpr TStringBuf BASS_TAXI_PUSH_POLICY = "bass-taxi-push";

constexpr TStringBuf SEARCH_FAILED_STATUS = "taxi_search_failed";
constexpr TStringBuf STATUS_SLOT_NAME = "status";
constexpr TStringBuf STRING_TYPE = "string";

constexpr TStringBuf PASSPORT_PHONES_LINK = "https://passport.yandex.ru/profile/phones";

struct TPushData {
    TStringBuilder Title;
    TStringBuilder Text;
    TStringBuilder Uri;
    TStringBuilder QuasarPayload;
    TStringBuilder QuasarEvent;
    TStringBuf Tag = TAXI_EVENT_TAG;
};

NSc::TValue GenerateActionPayload(TStringBuf formName, NSc::TValue slots = {}) {
    NSc::TValue form;
    form["name"] = "update_form";
    NSc::TValue& form_update = form["payload"]["form_update"];
    NSc::TValue& form_payload = form["payload"];
    form_update["name"].SetString(formName);
    if (slots.IsArray())
        form_update["slots"].CopyFrom(slots);
    else
        form_update["slots"].SetArray();
    form_payload["set_new_form"].SetBool(true);
    form_payload["resubmit"].SetBool(true);
    form_payload["session_state"].SetDict();
    form["type"] = "server_action";
    return form;
}

TString GenerateEmptyFormUri(TStringBuf formName, NSc::TValue slots = {}) {
    TCgiParameters cgi;
    NSc::TValue wrapper;
    wrapper.Push(GenerateActionPayload(formName, slots));
    cgi.InsertUnescaped("directives", wrapper.ToJson());
    return TStringBuilder{} << "dialog://?" << cgi.Print();
}

TString GenerateQuasarPayload(TStringBuf formName, NSc::TValue slots = {}) {
    NSc::TValue form = GenerateActionPayload(formName, slots);
    return form.ToJson();
}

void GetOnAssignedData(const NSc::TValue& serviceData, TPushData& pushData) {
    pushData.Title << DRIVER_FOUND;

    pushData.Text << "Такси приедет через " << serviceData["time_left"].ForceString() << " мин. " << serviceData["vehicle"].GetString() << ".";
    pushData.QuasarPayload << GenerateQuasarPayload(TAXI_STATUS_FORM);
    pushData.QuasarEvent << QUASAR_EVENT;
    pushData.Uri << GenerateEmptyFormUri(TAXI_STATUS_FORM);
}

void GetOnWaitingData(const NSc::TValue& serviceData, TPushData& pushData) {
    pushData.Title << DRIVER_ARRIVED;
    pushData.Text << "Такси ждёт вас. " << serviceData["vehicle"].GetString() << ".";
    pushData.QuasarPayload << GenerateQuasarPayload(TAXI_STATUS_FORM);
    pushData.QuasarEvent << QUASAR_EVENT;
    pushData.Uri << GenerateEmptyFormUri(TAXI_STATUS_FORM);
}

void GetOnArrivingData(const NSc::TValue& serviceData, TPushData& pushData) {
    pushData.Title << DRIVER_IS_NEAR;
    pushData.Text << "Время бесплатного ожидания — " << serviceData["free_waiting"].ForceIntNumber(3) << " мин.";
    pushData.QuasarPayload << GenerateQuasarPayload(TAXI_STATUS_FORM);
    pushData.QuasarEvent << QUASAR_EVENT;
    pushData.Uri << GenerateEmptyFormUri(TAXI_STATUS_FORM);
}

void GetOnDebtAllowed(const NSc::TValue&, TPushData& pushData) {
    pushData.Title << CANT_RESERVE_MONEY;
    pushData.Text << "Поезжайте и ни о чём не волнуйтесь, заплатите позже.";
    pushData.QuasarPayload << GenerateQuasarPayload(TAXI_STATUS_FORM);
    pushData.QuasarEvent << QUASAR_EVENT;
    pushData.Uri << GenerateEmptyFormUri(TAXI_STATUS_FORM);
}

void GetOnMovedToCash(const NSc::TValue&, TPushData& pushData) {
    pushData.Title << CANT_RESERVE_MONEY;
    pushData.Text << "Пожалуйста, оплатите поездку наличными.";
    pushData.QuasarPayload << GenerateQuasarPayload(TAXI_STATUS_FORM);
    pushData.QuasarEvent << QUASAR_EVENT;
    pushData.Uri << GenerateEmptyFormUri(TAXI_STATUS_FORM);
}

void GetOnMovedToCashWithCoupon(const NSc::TValue&, TPushData& pushData) {
    pushData.Title << CANT_RESERVE_MONEY;
    pushData.Text << "Оплатите поездку наличными. Цена пересчитана без скидки, используйте купон в следующий раз.";
    pushData.QuasarPayload << GenerateQuasarPayload(TAXI_STATUS_FORM);
    pushData.QuasarEvent << QUASAR_EVENT;
    pushData.Uri << GenerateEmptyFormUri(TAXI_STATUS_FORM);
}

void GetOnFailed(const NSc::TValue& serviceData, TPushData& pushData) {
    pushData.Title << DRIVER_DISCARDS_ORDER;
    pushData.Text << "Водитель " << serviceData["park"]["name"].GetString() << " отказался от заказа.";
    pushData.QuasarPayload << GenerateQuasarPayload(TAXI_STATUS_FORM);
    pushData.QuasarEvent << QUASAR_EVENT;
    pushData.Uri << GenerateEmptyFormUri(TAXI_STATUS_FORM);
}

void GetOnFailedPrice(const NSc::TValue& serviceData, TPushData& pushData) {
    pushData.Title << WAITING_TIMEOUT;
    auto finalCost = serviceData["final_cost"].IsNull() ? serviceData["cost"].ForceIntNumber() : serviceData["final_cost"].ForceIntNumber();
    pushData.Text << "Заказ отменён. C карты списано " << finalCost << " " << serviceData["currency"].GetString() << ".";
    pushData.QuasarPayload << GenerateQuasarPayload(TAXI_STATUS_FORM);
    pushData.QuasarEvent << QUASAR_EVENT;
    pushData.Uri << GenerateEmptyFormUri(TAXI_STATUS_FORM);
}

void GetFailedData(const NSc::TValue&, TPushData& pushData) {
    NSc::TValue slots;
    NSc::TValue& statusSlot = slots.Push();
    statusSlot["name"] = STATUS_SLOT_NAME;
    statusSlot["optional"].SetBool(true);
    statusSlot["type"] = STRING_TYPE;
    statusSlot["value"] = SEARCH_FAILED_STATUS;
    pushData.Title << ORDER_FAILED;
    pushData.Text << "Попробуйте сделать заказ снова.";
    pushData.QuasarPayload << GenerateQuasarPayload(TAXI_ORDER, slots);
    pushData.QuasarEvent << QUASAR_EVENT;
    pushData.Uri << GenerateEmptyFormUri(TAXI_ORDER, slots);
}

// Local events
void GetSendLegalData(const NSc::TValue&, TPushData& pushData) {
    pushData.Title << ALICE;
    pushData.Text << "Условия использования Яндекс.Такси. Это важно.";
    pushData.Uri << GenerateEmptyFormUri(TAXI_SHOW_LEGAL);
    pushData.Tag = TAXI_LEGAL_TAG;
}

void GetCallToDriverData(const NSc::TValue&, TPushData& pushData) {
    pushData.Title << ALICE;
    pushData.Text << "Звоним водителю? Нажмите, если да.";
    pushData.Uri << GenerateEmptyFormUri(TAXI_CALL_TO_DRIVER);
    pushData.Tag = TAXI_LOCAL_TAG;
}

void GetCallToSuportData(const NSc::TValue&, TPushData& pushData) {
    pushData.Title << ALICE;
    pushData.Text << "Жмите сюда, чтобы пообщаться с поддержкой Такси.";
    pushData.Uri << GenerateEmptyFormUri(TAXI_CALL_TO_SUPPORT);
    pushData.Tag = TAXI_LOCAL_TAG;
}

void GetWhoIsTransporterData(const NSc::TValue&, TPushData& pushData) {
    pushData.Title << ALICE;
    pushData.Text << "Вот информация о вашем перевозчике.";
    pushData.Uri << GenerateEmptyFormUri(TAXI_SHOW_DRIVER_INFO);
    pushData.Tag = TAXI_DRIVER_INFO;
}

void GetAddPhoneInPassportData(const NSc::TValue&, TPushData& pushData) {
    pushData.Title << ALICE;
    pushData.Text << "Добавим номер телефона к аккаунту?";
    pushData.Uri << PASSPORT_PHONES_LINK;
    pushData.Tag = TAXI_LOCAL_TAG;
}
} // namespace

TResultValue TTaxiPush::Generate(THandler& handler, TApiSchemeHolder scheme) {
    if (EVENTS.find(scheme->Event()) != EVENTS.end()) {
        TPushData pushData{};
        int ttl = 500;
        const NSc::TValue& serviceData = *scheme->ServiceData().GetRawValue();
        if (scheme->Event() == ON_ASSIGNED || scheme->Event() == ON_ASSIGNED_EXACT) {
            GetOnAssignedData(serviceData, pushData);
        } else if (scheme->Event() == ON_WAITING) {
            GetOnWaitingData(serviceData, pushData);
        } else if (scheme->Event() == ON_SEARCH_FAILED ||  scheme->Event() == ON_AUTOREORDER_TIMEOUT) {
            GetFailedData(serviceData, pushData);
        } else if (scheme->Event() == ON_DRIVER_ARRIVING) {
            GetOnArrivingData(serviceData, pushData);
        } else if (scheme->Event() == ON_DEBT_ALLOWED) {
            GetOnDebtAllowed(serviceData, pushData);
        } else if (scheme->Event() == ON_MOVED_TO_CASH) {
            GetOnMovedToCash(serviceData, pushData);
        } else if (scheme->Event() == ON_MOVED_TO_CASH_WITH_COUPON) {
            GetOnMovedToCashWithCoupon(serviceData, pushData);
        } else if (scheme->Event() == ON_FAILED) {
            GetOnFailed(serviceData, pushData);
        } else if (scheme->Event() == ON_FAILED_PRICE || scheme->Event() == ON_FAILED_PRICE_WITH_COUPON) {
            GetOnFailedPrice(serviceData, pushData);
        } else if (scheme->Event() == LOCAL_SEND_LEGAL) {
            GetSendLegalData({}, pushData);
        } else if (scheme->Event() == LOCAL_CALL_TO_DRIVER) {
            GetCallToDriverData({}, pushData);
        } else if (scheme->Event() == LOCAL_CALL_TO_SUPPORT) {
            GetCallToSuportData({}, pushData);
        } else if (scheme->Event() == LOCAL_WHO_IS_TRANSPORTER) {
            GetWhoIsTransporterData({}, pushData);
        } else if (scheme->Event() == LOCAL_ADD_PHONE_IN_PASSPORT) {
            GetAddPhoneInPassportData({}, pushData);
        }
        handler.SetInfo(scheme->Event(), pushData.QuasarEvent, pushData.Title, pushData.Text, pushData.Uri, pushData.Tag, ttl, pushData.QuasarPayload, BASS_TAXI_PUSH_POLICY);

        if (handler.GetClientInfo().IsSmartSpeaker()) {
            handler.AddCustom("ru.yandex.mobile");
            handler.AddCustom("ru.yandex.mobile.inhouse");
            handler.AddCustom("ru.yandex.mobile.dev");
            handler.AddCustom("ru.yandex.searchplugin");
            handler.AddCustom("ru.yandex.searchplugin.dev");
            handler.AddCustom("ru.yandex.searchplugin.beta");
            handler.AddCustom("ru.yandex.searchplugin.nightly");
            handler.AddCustom("ru.yandex.weatherplugin");
        }
        if (!handler.GetClientInfo().IsSmartSpeaker() || !pushData.QuasarPayload.empty()) {
            handler.AddSelf();
        }
        return ResultSuccess();
    }
    return TError{TError::EType::INVALIDPARAM,
                  TStringBuilder{} << "no handler found for '" << scheme->Service() << "' and event '" << scheme->Event() << '\''
    };
}

} // namespace NBASS::NPushNotification
