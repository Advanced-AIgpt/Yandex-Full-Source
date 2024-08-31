//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportApi](index.md)/[logout](logout.md)

# logout

[passport]\
abstract suspend fun [logout](logout.md)(uid: [PassportUid](../-passport-uid/index.md)): Result&lt;Unit&gt;

Метод требуется вызывать в случае, если пользователь нажал &quot;Выйти&quot; в приложении

Выполняет следующие действия:

<ui>
* Удаляет закэшированный клиентский токен
* Отключает автологин для данного аккаунта. [.tryAutoLogin]
* В этом методе **не происходит** отзыв токена на сервере, только локальные действия с токеном,
* но в некоторых случах обращение в сеть может происходить
</ui>

## See also

passport

| | |
|---|---|
|  | .dropToken |

## Parameters

passport

| | |
|---|---|
| uid | [PassportUid](../-passport-uid/index.md) аккаунта из которого пользователь вышел |

## Throws

| | |
|---|---|
| [com.yandex.passport.api.exception.PassportRuntimeUnknownException](../../com.yandex.passport.api.exception/-passport-runtime-unknown-exception/index.md) |  |
