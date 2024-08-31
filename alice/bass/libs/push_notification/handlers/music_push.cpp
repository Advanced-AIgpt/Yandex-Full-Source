#include "music_push.h"

namespace NBASS::NPushNotification {
TStringBuf LINK_FOR_BUY_PLUS_EVENT = "link_for_buy_plus";
TStringBuf LINK_FOR_BUY_PLUS_TITLE = "Алиса";
TStringBuf LINK_FOR_BUY_PLUS_DESCRIPTION = "Держите ссылку для подключения Плюса";
TStringBuf LINK_FOR_BUY_PLUS_URL = "https://plus.yandex.ru/";


TStringBuf THROTTLE_POLICY = "bass-music";


TResultValue TMusicPush::Generate(THandler& handler, TApiSchemeHolder scheme) {
    if (scheme->Event() == LINK_FOR_BUY_PLUS_EVENT) {
        handler.SetInfo(
            scheme->Event(),
            "" /*quasar event*/,
            LINK_FOR_BUY_PLUS_TITLE,
            LINK_FOR_BUY_PLUS_DESCRIPTION,
            LINK_FOR_BUY_PLUS_URL,
            LINK_FOR_BUY_PLUS_EVENT /*tag*/,
            5000 /*TTL*/,
            "" /*quasar payload*/,
            THROTTLE_POLICY
        );

        if (handler.GetClientInfo().IsSmartSpeaker()) {
            handler.AddCustom("ru.yandex.mobile");
            handler.AddCustom("ru.yandex.mobile.inhouse");
            handler.AddCustom("ru.yandex.mobile.dev");
            handler.AddCustom("ru.yandex.searchplugin");
            handler.AddCustom("ru.yandex.searchplugin.dev");
            handler.AddCustom("ru.yandex.searchplugin.beta");
            handler.AddCustom("ru.yandex.searchplugin.nightly");
            handler.AddCustom("ru.yandex.weatherplugin");
        } else {
            handler.AddSelf();
        }
        return ResultSuccess();
    }
    return TError{TError::EType::INVALIDPARAM,
                  TStringBuilder{} << "no handler found for '" << scheme->Service() << "' and event '" << scheme->Event() << '\''
    };
}

} // namespace NBASS::NPushNotification
