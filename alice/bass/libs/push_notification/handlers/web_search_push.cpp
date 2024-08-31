#include "web_search_push.h"

namespace NBASS::NPushNotification {
namespace {
TStringBuf SEARCH_PUSH_EVENT = "web_search";
TStringBuf SEARCH_PUSH_TITLE = "Алиса";
TStringBuf SEARCH_PUSH_DESCRIPTION = "Искать ";

} // namespace

TResultValue TWebSearchPush::Generate(THandler& handler, TApiSchemeHolder scheme) {
    if (scheme->Event() == SEARCH_PUSH_EVENT) {
        const NSc::TValue& serviceData = *scheme->ServiceData().GetRawValue();

        handler.SetInfo(SEARCH_PUSH_EVENT, "" /*quasar event*/, SEARCH_PUSH_TITLE,
                        TStringBuilder() << SEARCH_PUSH_DESCRIPTION << serviceData["query"].GetString(),
                        serviceData["url"], "alice_web_search_push" /*tag*/, 360 /*TTL*/, "" /*quasar payload*/);

        handler.AddCustom("ru.yandex.mobile");
        handler.AddCustom("ru.yandex.mobile.inhouse");
        handler.AddCustom("ru.yandex.mobile.dev");
        handler.AddCustom("ru.yandex.searchplugin");
        handler.AddCustom("ru.yandex.searchplugin.dev");
        handler.AddCustom("ru.yandex.searchplugin.beta");
        handler.AddCustom("ru.yandex.searchplugin.nightly");

        return ResultSuccess();
    }

    return TError{TError::EType::INVALIDPARAM, TStringBuilder{} << "no handler found for '" << scheme->Service()
                                                                << "' and event '" << scheme->Event() << '\''};
}

} // namespace NBASS::NPushNotification
