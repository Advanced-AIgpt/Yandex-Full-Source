//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportApi](index.md)/[logout](logout.md)

# logout

[passport]\

@WorkerThread

abstract fun [logout](logout.md)(@NonNulluid: [PassportUid](../-passport-uid/index.md))

 Метод требуется вызывать в случае, если пользователь нажал &quot;Выйти&quot; в приложении 

 Выполняет следующие действия: 

Удаляет закэшированный клиентский токенОтключает автологин для данного аккаунта. [tryAutoLogin](try-auto-login.md)В этом методе **не происходит** отзыв токена на сервере, только локальные действия с токеном, но в некоторых случах обращение в сеть может происходить

## See also

passport

| | |
|---|---|
| [tryAutoLogin(PassportAutoLoginProperties)](try-auto-login.md) |  |
| [dropToken(String)](drop-token.md) |  |

## Parameters

passport

| | |
|---|---|
| uid | [PassportUid](../-passport-uid/index.md) аккаунта из которого пользователь вышел |

## Throws

| | |
|---|---|
| [com.yandex.passport.api.exception.PassportRuntimeUnknownException](../../com.yandex.passport.api.exception/-passport-runtime-unknown-exception/index.md) |  |
