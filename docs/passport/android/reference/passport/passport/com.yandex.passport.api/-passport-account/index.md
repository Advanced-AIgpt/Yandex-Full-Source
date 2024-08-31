//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportAccount](index.md)

# PassportAccount

[passport]\
interface [PassportAccount](index.md)

Содержит описание аккаунта в Паспорте, а также некоторых локальных для устройства параметров (пин-код). См.также описание [серверного API](https://wiki.yandex-team.ru/passport/python/api/bundle/account/#polucheniekratkojjinformaciiobakkaunte).

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.PassportUid](../-passport-uid/index.md) |  |
| [com.yandex.passport.api.PassportStash](../-passport-stash/index.md) |  |

## Functions

| Name | Summary |
|---|---|
| [getAndroidAccount](get-android-account.md) | [passport]<br>@NonNull<br>abstract fun [getAndroidAccount](get-android-account.md)(): Account<br>Возвращает андроидный аккаунт для использования с API Android. |
| [getAvatarUrl](get-avatar-url.md) | [passport]<br>@Nullable<br>abstract fun [getAvatarUrl](get-avatar-url.md)(): String<br>URL картинки с аватаркой. |
| [getBirthday](get-birthday.md) | [passport]<br>@Nullable<br>abstract fun [getBirthday](get-birthday.md)(): Date |
| [getFirstName](get-first-name.md) | [passport]<br>@Nullable<br>abstract fun [getFirstName](get-first-name.md)(): String |
| [getLastName](get-last-name.md) | [passport]<br>@Nullable<br>abstract fun [getLastName](get-last-name.md)(): String |
| [getNativeDefaultEmail](get-native-default-email.md) | [passport]<br>@Nullable<br>abstract fun [getNativeDefaultEmail](get-native-default-email.md)(): String<br>Нативный email адрес по-умолчанию. |
| [getPrimaryDisplayName](get-primary-display-name.md) | [passport]<br>@NonNull<br>abstract fun [getPrimaryDisplayName](get-primary-display-name.md)(): String<br>Главное отображаемое имя пользователя. |
| [getPublicId](get-public-id.md) | [passport]<br>@Nullable<br>abstract fun [getPublicId](get-public-id.md)(): String<br>[Подробнее про public_id ](https://wiki.yandex-team.ru/users/olegmatykov/custom-publicid/) |
| [getSecondaryDisplayName](get-secondary-display-name.md) | [passport]<br>@Nullable<br>abstract fun [getSecondaryDisplayName](get-secondary-display-name.md)(): String<br>Второе отображаемое имя пользователя. |
| [getSocialProviderCode](get-social-provider-code.md) | [passport]<br>@Nullable<br>abstract fun [getSocialProviderCode](get-social-provider-code.md)(): String<br>Код соц. |
| [getStash](get-stash.md) | [passport]<br>@NonNull<br>abstract fun [getStash](get-stash.md)(): [PassportStash](../-passport-stash/index.md)<br>Получает объект, реализующий интерфейс [PassportStash](../-passport-stash/index.md), у которого можно узнать сохранённые значения по ключу из [PassportStashCell](../-passport-stash-cell/index.md). |
| [getUid](get-uid.md) | [passport]<br>@NonNull<br>abstract fun [getUid](get-uid.md)(): [PassportUid](../-passport-uid/index.md)<br>uid – уникальный идентификатор, однозначно указывающий на определённый аккаунт в базе Паспорта. |
| [hasPlus](has-plus.md) | [passport]<br>abstract fun [hasPlus](has-plus.md)(): Boolean<br>Возвращает статус подписки Yandex.Plus. |
| [isAuthorized](is-authorized.md) | [passport]<br>abstract fun [isAuthorized](is-authorized.md)(): Boolean<br>true – на устройстве присутствует авторизационный токен (&quot;мастер&quot; токен), по которому можно попытаться получить [токен приложения](../-passport-token/index.md) (&quot;клиентский&quot; токен). |
| [isAvatarEmpty](is-avatar-empty.md) | [passport]<br>abstract fun [isAvatarEmpty](is-avatar-empty.md)(): Boolean<br>true – картинка с аватаркой [getAvatarUrl](get-avatar-url.md) содержит заглушку. |
| [isBetaTester](is-beta-tester.md) | [passport]<br>abstract fun [isBetaTester](is-beta-tester.md)(): Boolean<br>true – пользователь помечен как бета-тестировщик. |
| [isLite](is-lite.md) | [passport]<br>abstract fun [isLite](is-lite.md)(): Boolean<br>true – &quot;лайт&quot; аккаунт. |
| [isMailish](is-mailish.md) | [passport]<br>abstract fun [isMailish](is-mailish.md)(): Boolean<br>true – &quot;мейлиш&quot; аккаунт. |
| [isPdd](is-pdd.md) | [passport]<br>abstract fun [isPdd](is-pdd.md)(): Boolean<br>true – аккаунт &quot;паспорт для домена&quot;. |
| [isPhonish](is-phonish.md) | [passport]<br>abstract fun [isPhonish](is-phonish.md)(): Boolean<br>true – &quot;фониш&quot; аккаунт. |
| [isSocial](is-social.md) | [passport]<br>abstract fun [isSocial](is-social.md)(): Boolean<br>true – социальный аккаунт. |
| [isYandexoid](is-yandexoid.md) | [passport]<br>abstract fun [isYandexoid](is-yandexoid.md)(): Boolean<br>true – аккаунт является аккаунтом сотрудника Яндекса. |
