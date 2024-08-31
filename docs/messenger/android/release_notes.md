# Релизы

Все релизы sdk лежат в [artifactory](http://artifactory.yandex.net/artifactory/yandex_mobile_releases/com/yandex/messenger/messaging/)

## Версия 143.0
- включили фичу разговорных нотификаций (контролируется `MessagingConfiguration#areConversationsEnabled`, по умолчанию `false`)
- удалили `LimitedAccessConfiguration#httpClientProvider`
- удалили `MessengerSdk#synchronizeCurrentProfileFullState`
- удалили `MessengerSdk#subscribeChatMuteState`
- удалили `MessengerSdk#chatMuteStateFlow`
- удалили `MessengerSdkWrapper#subscribeChatMuteState`

## Версия 136.0
В `MessagingConfiguration` добавили необязательное поле `httpClientBuilder`, позволяющее модифицировать `OkHttpClient.Builder`.
При этом `LimitedAccessConfiguration#httpClientProvider` теперь помечено как `Deprecated` (обратите внимание, что в новом апи не надо билдить `OkHttpClient`).

## Версия 135.0
Включили флаг отображения соответвующих нотификаций в conversation-формате.
Управлять можно через `MessengerSdkConfiguration#areConversationsEnabled` (по умолчанию в `false`).

## Версия 134.0
Добавили в PublicApi `MessengerSdkConfiguration` параметр `areConversationsEnabled`, позволяющий включать отображение соответвующих нотификаций в conversation-формате.
Пока что функциональность за флагом, так что переключение параметра ни на что не влияет.
Сейчас параметр по умолчанию в `false`, в будущих релизах будет `true`.

## Версия 128.0
#### Изменения в PublicApi
- Задепрекейтили `MetricaManager.MetricaIdentityProvider`. Чтобы перепоределить uuid и deviceId передайте свою реализацию `com.yandex.alicekit.core.IdentityProvider` в `MessengerSdkConfiguration.identityProvider`.
- Метод `IdentityProvider.getPackageNameForXiva` пометили как `@Deprecated`. В скором времени данный метод будет удален. Вместо него используйте новый метод `CloudMessagingProvider.getPackageNameForXiva`. 
- Атрибут темы `messagingOnSurfaceColor` теперь `@Deprecated`. Рекомендуем использовать вместе него `messagingCommonAccentFgColor`.

## Версия 127.0

#### Изменения в PublicApi

 - Добавили в PublicApi `MessengerSdkConfiguration` возможность хостам выставлять `MetricaInterceptor`, в который будут прилетать все события мессенджера.
 - Задеприкейтили переопределение и отключение встроенной аналитики мессенджера `MetricaProvider` в пользу использования `MetricaInterceptor`
## Версия 126.0

 - Добавили новый цвет в палитру messagingOnSurfaceColor (используется для всех шрифтов и иконок, которые находятся на подложке из messagingCommonAccentColor)

## Версия 125.0

#### Изменения в PublicApi WebSdk

 - Добавили кастомный обработичк клика по нотификации `NotificationClickIntentFactory` 
 - Обработка клика по нотификации через `MessengerParams.notificationClickAction` и вспомогательный к нему метод `Notification.parseTappedNotification` помечены как `@Deprecated` и в ближайшие релизы будут удалены. 
 - Добавили новые цвета в палитру: `messagingCommonAccentTransparent25PercentColor` и `messagingPollsBackgroundColor` (на данный момент используется только как цвет фона варианта ответа в опросе)
   


## Версия 124.0


## Версия 123.0


#### Изменения в PublicApi
- Метод `IdentityProvider.getGcmSenderId` пометили как `@Deprecated`, т.к в сдк давно не используется. В скором времени данный метод будет удален. 
- Удалили функционал "комментариев на сайтах". Вызов `MessengerSdk.createSiteCommentsFetcher` теперь возвращает NoOp имплементацию. В скором времени метод и весь связанный интерфейс будет удален. 

## Версия 122.0


#### Изменения в PublicApi

Добавили возможность менять маленькую иконку в нотификациях:
`MessengerSdkConfiguration` -> `ChatNotificationDecorator` -> `smallIcon()`

## Версия 121.0


## Версия 120.0


## Версия 119.0


#### Изменения в PublicApi

Оторвали зависимость паспорта. Теперь используются классы-обертки для работы с паспортовыми сущностями.

Для конвертации сущностей добавили экстеншены: `com.yandex.messaging.auth.passport.converters`

- `MessengerSdk#onAccountChanged` теперь принимает `AuthUid` вместо `PassportUid`.
**Миграция**: `passportUid` -> `passportUid.toAuthUid()`
- коллбек `AccountProvider#onAccountChanged` теперь тоже принимает `AuthUid` вместо `PassportUid`.
**Миграция**: `uid` -> `uid.toPassportUid()`
- `MessengerSdkConfiguration` теперь опционально принимает параметр `authApi: AuthApi?`. По умолчанию использует паспортовую реализацию, так что, если планируете использовать паспорт, **менять ничего не надо**.
**Если нужна своя реализация** – можно реализовать интерфейс `AuthApi` или наследоваться от открытого класса `PassportAuthApi`.
**Для отключения авторизации** передайте `null`. При этом действия в интерфейсе, требующие авторизацию, будут игнорироваться.
