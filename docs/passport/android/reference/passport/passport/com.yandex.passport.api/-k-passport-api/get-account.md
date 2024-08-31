//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportApi](index.md)/[getAccount](get-account.md)

# getAccount

[passport]\
abstract suspend fun [getAccount](get-account.md)(uid: [PassportUid](../-passport-uid/index.md)): Result&lt;[PassportAccount](../-passport-account/index.md)&gt;

Возвращает аккаунт [PassportAccount](../-passport-account/index.md) по уникальному идентификатору [PassportUid](../-passport-uid/index.md)<br></br>

- 
   Будет найден аккаунт с нужным uid.
- 
   В качестве источника аккаунтов используется системная база – результат вызова &quot;com.yandex.passport&quot;.
- 
   В процессе возможно восстановление из внутреннего бекапа и обновление данных в системной базе, в частности при отсутствии uid у одного из аккаунтов он может быть получен из сети по авторизационному токену. <br></br>

#### Return

аккаунт [PassportAccount](../-passport-account/index.md)

## Parameters

passport

| | |
|---|---|
| uid | [PassportUid](../-passport-uid/index.md) для поиска |

## Throws

| | |
|---|---|
| [com.yandex.passport.api.exception.PassportAccountNotFoundException](../../com.yandex.passport.api.exception/-passport-account-not-found-exception/index.md) | аккаунт с таким uid не найден |
| [com.yandex.passport.api.exception.PassportRuntimeUnknownException](../../com.yandex.passport.api.exception/-passport-runtime-unknown-exception/index.md) | внутренняя ошибка |
