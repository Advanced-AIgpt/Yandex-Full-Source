//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportApi](index.md)/[removeAccount](remove-account.md)

# removeAccount

[passport]\

@WorkerThread

abstract fun [removeAccount](remove-account.md)(@NonNulluid: [PassportUid](../-passport-uid/index.md))

 &quot;Настоящее&quot; удаление аккаунта и связанных с ним данных из системной базы. 

 Если вам кажется, что ваше приложение нуждается в удалении аккаунтов - обратитесь к команде разработчиков библиотеки passport. 

**Использование метода ограничено Яндекс.Почтой, Яндекс.Диском, Яндекс.Авто**

#### Deprecated

## Parameters

passport

| | |
|---|---|
| uid | [PassportUid](../-passport-uid/index.md) удаляемого аккаунта |

## Throws

| | |
|---|---|
| [com.yandex.passport.api.exception.PassportRuntimeUnknownException](../../com.yandex.passport.api.exception/-passport-runtime-unknown-exception/index.md) | ,PassportAccountNotFoundException |
