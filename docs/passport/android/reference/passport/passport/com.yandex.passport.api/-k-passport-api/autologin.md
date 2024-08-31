//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportApi](index.md)/[autologin](autologin.md)

# autologin

[passport]\
abstract suspend fun [autologin](autologin.md)(activity: ComponentActivity, useSmartlock: Boolean, properties: [PassportAutoLoginProperties](../-passport-auto-login-properties/index.md)): Result&lt;[PassportAccount](../-passport-account/index.md)&gt;

abstract suspend fun [autologin](autologin.md)(activity: ComponentActivity, useSmartlock: Boolean, properties: [PassportAutoLoginProperties.Builder](../-passport-auto-login-properties/-builder/index.md).() -&gt; Unit): Result&lt;[PassportAccount](../-passport-account/index.md)&gt;

Производит поиск подходящего для приложения аккаунта в системном хранилище.

Если в системном хранилище нет аккаунтов, то метод попытается получить аккаунт из google smartlock.

В случае если в smartlock сохранен только один аккаунт, то внизу экрана отобразится синий снэкбар (отображается средствами google play services)

После получения логина и пароля из смартлока производится попытка авторизации.

В случае необходимости показываются дополнительные активити. Хосту обрабатывать их не нужно.

Алгоритм поиска подходящего аккаунта:

- 
   Загружаются все аккаунты, присутствующие не девайсе
- 
   Отфильтровываются аккаунты, подходящие под переданный фильтр [PassportAutoLoginProperties.Builder.setFilter](../-passport-auto-login-properties/-builder/set-filter.md)
- 
   Аккаунты сортируются по следующему приоритету:
- 
   Портальные
- 
   Социальные
- 
   ПДД
- 
   yandex-team
- 
   Остальные аккаунты (за исключением фонишей)
- 
   Производится поиск первого аккаунта, подходящего под следующие критерии:
- 
   Автологин для данного аккаунта **не** отключен
- 
   Для данного аккаунта успешно получен **новый** клиентский токен из сети

Автологин может быть отключен для аккаунта в случае если приложение для него вызвало метод .logout или пользователь нажал &quot;Выйти&quot; в Activity .createAutoLoginIntent

#### Return

подходящий для приложения аккаунт для автологина

## See also

passport

| | |
|---|---|
|  | .createAutoLoginIntent |
| [com.yandex.passport.api.PassportAutoLoginProperties](../-passport-auto-login-properties/index.md) |  |

## Parameters

passport

| | |
|---|---|
| activity | ComponentActivity для использования в качестве Context и запуска дополнительных активити через механизм ActivityResultContract. |
| useSmartlock | enable/disable smartlock aulologin |
| properties | параметры для поиска подходящего аккаунта |

## Throws

| | |
|---|---|
| [com.yandex.passport.api.exception.PassportAutoLoginImpossibleException](../../com.yandex.passport.api.exception/-passport-auto-login-impossible-exception/index.md) | -     если подходящий аккаунт не найден, либо не получилось получить получить новый токен для аккаунтов |
| [com.yandex.passport.api.exception.PassportRuntimeUnknownException](../../com.yandex.passport.api.exception/-passport-runtime-unknown-exception/index.md) |  |
