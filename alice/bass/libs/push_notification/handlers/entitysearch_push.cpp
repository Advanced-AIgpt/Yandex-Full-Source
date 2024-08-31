#include "entitysearch_push.h"

#include <library/cpp/scheme/scheme.h>

namespace NBASS::NPushNotification {
TResultValue TEntitySearchPush::Generate(THandler& handler, TApiSchemeHolder scheme) {
    if (scheme->Event() == EVENT_BUY_TICKET) {
        const NSc::TValue& request = *scheme->ServiceData().GetRawValue();
        TStringBuf sessionId = request["session_id"].GetString();
        TStringBuf text = request["text"].GetString();
        if (sessionId.Empty()) {
            return TError{
                TError::EType::INVALIDPARAM,
                TStringBuilder{} << "session_id is not set for '" << scheme->Service() << "' and event '" << scheme->Event() << '\''};
        }
        TString url = TStringBuilder{} << "https://widget.afisha.yandex.ru/w/sessions/" << sessionId;
        handler.SetInfo(
            scheme->Event(),
            "" /* quasar event */,
            "Алиса",
            text ? text : "Держите ссылку для покупки билетов в кино",
            url,
            "alice_entity_afisha_push" /* tag */,
            500 /* TTL */,
            "" /* quasar payload */,
            "bass-entity-afisha-push" /* throttle policy */
        );

        if (handler.GetClientInfo().IsSmartSpeaker() || handler.GetUUId().empty()) {
            handler.AddCustom("ru.yandex.mobile.search.ipad");
            handler.AddCustom("ru.yandex.mobile.search");
            handler.AddCustom("com.yandex.browser");
            handler.AddCustom("com.yandex.browser.beta");
            handler.AddCustom("ru.yandex.mobile");
            handler.AddCustom("ru.yandex.mobile.inhouse");
            handler.AddCustom("ru.yandex.mobile.dev");
            handler.AddCustom("ru.yandex.searchplugin");
            handler.AddCustom("ru.yandex.searchplugin.dev");
            handler.AddCustom("ru.yandex.searchplugin.beta");
            handler.AddCustom("ru.yandex.searchplugin.nightly");
        } else {
            handler.AddSelf();
        }
        return ResultSuccess();
    }
    return TError{TError::EType::INVALIDPARAM,
                  TStringBuilder{} << "no handler found for '" << scheme->Service() << "' and event '" << scheme->Event() << '\''};
}
} // namespace NBASS::NPushNotification
