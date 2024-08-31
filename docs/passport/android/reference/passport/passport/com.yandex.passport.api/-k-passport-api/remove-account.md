//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportApi](index.md)/[removeAccount](remove-account.md)

# removeAccount

[passport]\
~~abstract~~ ~~suspend~~ ~~fun~~ [~~removeAccount~~](remove-account.md)~~(~~uid: [PassportUid](../-passport-uid/index.md)~~)~~~~:~~ Result&lt;Unit&gt;

&quot;Настоящее&quot; удаление аккаунта и связанных с ним данных из системной базы.

Если вам кажется, что ваше приложение нуждается в удалении аккаунтов - обратитесь к команде разработчиков библиотеки passport.

## Parameters

passport

| | |
|---|---|
| uid | [PassportUid](../-passport-uid/index.md) удаляемого аккаунта |

## Throws

| | |
|---|---|
| [com.yandex.passport.api.exception.PassportRuntimeUnknownException](../../com.yandex.passport.api.exception/-passport-runtime-unknown-exception/index.md) | , PassportAccountNotFoundException |
