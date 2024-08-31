#include "onboarding_push.h"

namespace NBASS::NPushNotification {

namespace {

constexpr TStringBuf TITLE = "Алиса";
constexpr TStringBuf URL = "http://dialogs.yandex.ru/store/essentials?surface=station&utm_source=push-what-can-you-do";

static const TMap<TStringBuf, TStringBuf> TEXTS = {
        {TStringBuf("1"), TStringBuf("Посмотрите, какие задачи я умею решать")},
        {TStringBuf("2"), TStringBuf("Посмотрите, чем я могу вам помочь")},
        {TStringBuf("3"), TStringBuf("Посмотрите, о чем вы можете просить меня каждый день")},
        {TStringBuf("4"), TStringBuf("Давайте познакомимся поближе? Я могу быть очень полезной. Смотрите...")}
};

constexpr TStringBuf TEXT_DEFAULT_KEY = "1";

} //namespace anonymous

TResultValue TOnboardingPush::Generate(THandler& handler, TApiSchemeHolder scheme) {
    const NSc::TValue& serviceData = *scheme->ServiceData().GetRawValue();
    const TStringBuf textNum = serviceData["text_num"].GetString(TEXT_DEFAULT_KEY);
    if (scheme->Event() == ONBOARDING_EVENT) {
        handler.SetInfo(
            ONBOARDING_EVENT, //event
            "", //quasarEvent
            TITLE,
            TEXTS.at(TEXTS.contains(textNum) ? textNum : TEXT_DEFAULT_KEY),
            URL,
            ONBOARDING_EVENT, //tag
            500 //ttl
        );
        handler.AddCustom("ru.yandex.mobile");
        handler.AddCustom("ru.yandex.mobile.inhouse");
        handler.AddCustom("ru.yandex.mobile.dev");
        handler.AddCustom("ru.yandex.searchplugin");
        handler.AddCustom("ru.yandex.searchplugin.dev");
        handler.AddCustom("ru.yandex.searchplugin.beta");
        handler.AddCustom("ru.yandex.searchplugin.nightly");
        handler.AddCustom("ru.yandex.weatherplugin");
        return ResultSuccess();
    }
    return TError{TError::EType::INVALIDPARAM,
                  TStringBuilder{} << "no handler found for '" << scheme->Service() << "' and event '" << scheme->Event() << '\''
    };
}

} // namespace NBASS::NPushNotification

