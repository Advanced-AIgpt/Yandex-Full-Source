//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportApi](index.md)/[tryAutoLogin](try-auto-login.md)

# tryAutoLogin

[passport]\

@WorkerThread

@CheckResult

@NonNull

@Deprecated

~~abstract~~ ~~fun~~ [~~tryAutoLogin~~](try-auto-login.md)~~(~~@NonNullautoLoginProperties: [PassportAutoLoginProperties](../-passport-auto-login-properties/index.md)~~)~~~~:~~ [PassportAccount](../-passport-account/index.md)

 Производит поиск подходящего для приложения аккаунта в системном хранилище. 

 Алгоритм поиска подходящего аккаунта: 

- Загружаются все аккаунты, присутствующие на устройстве
- Отфильтровываются аккаунты, подходящие под переданный фильтр [setFilter](../../../../passport/passport/com.yandex.passport.api/-passport-auto-login-properties/-builder/set-filter.md)
- Аккаунты сортируются по следующему приоритету: 
   
   - Портальные
   - Социальные
   - ПДД
   - yandex-team
   - Остальные аккаунты (за исключением фонишей)
- Производится поиск первого аккаунта, подходящего под следующие критерии: 
   
   - Автологин для данного аккаунта **не** отключен
   - Для данного аккаунта успешно получен **новый** клиентский токен из сети

 Автологин может быть отключен для аккаунта в случае если приложение для него вызвало метод [logout](logout.md) или пользователь нажал &quot;Выйти&quot; в Activity [createAutoLoginIntent](create-auto-login-intent.md)

#### Return

подходящий для приложения аккаунт для автологина

#### Deprecated

Используйте [tryAutoLogin](try-auto-login.md)

## See also

passport

| | |
|---|---|
| [logout(PassportUid)](logout.md) |  |
| [createAutoLoginIntent(Context, PassportUid, PassportAutoLoginProperties)](create-auto-login-intent.md) |  |
| [com.yandex.passport.api.PassportAutoLoginProperties](../-passport-auto-login-properties/index.md) |  |

## Parameters

passport

| | |
|---|---|
| autoLoginProperties | параметры для поиска подходящего аккаунта |

## Throws

| | |
|---|---|
| [com.yandex.passport.api.exception.PassportAutoLoginImpossibleException](../../com.yandex.passport.api.exception/-passport-auto-login-impossible-exception/index.md) | - если подходящий аккаунт не найден, либо не получилось получить получить новый токен для аккаунтов |
| [com.yandex.passport.api.exception.PassportRuntimeUnknownException](../../com.yandex.passport.api.exception/-passport-runtime-unknown-exception/index.md) |  |

[passport]\

@WorkerThread

@CheckResult

@NonNull

abstract fun [tryAutoLogin](try-auto-login.md)(@NonNullcontext: Context, @NonNullproperties: [PassportAutoLoginProperties](../-passport-auto-login-properties/index.md)): [PassportAutoLoginResult](../-passport-auto-login-result/index.md)

 Производит поиск подходящего для приложения аккаунта в системном хранилище. 

 Если в системном хранилище нет аккаунтов, то метод попытается получить аккаунт из google smartlock. 

 В случае если в smartlock сохранен только один аккаунт, то внизу экрана отобразится синий снэкбар (отображается средствами google play services) 

 После получения логина и пароля из смартлока производится попытка авторизации. В случае успешной попытки возвращается результат [PassportAutoLoginResult](../-passport-auto-login-result/index.md) в котором хранится PassportAccount. Если попытка не была успешна, выбрасывается исключение, которое содержит Intent, его нужно показать через startActivityForResult. 

 Алгоритм поиска подходящего аккаунта: 

- Загружаются все аккаунты, присутствующие не девайсе
- Отфильтровываются аккаунты, подходящие под переданный фильтр [setFilter](../../../../passport/passport/com.yandex.passport.api/-passport-auto-login-properties/-builder/set-filter.md)
- Аккаунты сортируются по следующему приоритету: 
   
   - Портальные
   - Социальные
   - ПДД
   - yandex-team
   - Остальные аккаунты (за исключением фонишей)
- Производится поиск первого аккаунта, подходящего под следующие критерии: 
   
   - Автологин для данного аккаунта **не** отключен
   - Для данного аккаунта успешно получен **новый** клиентский токен из сети

 Автологин может быть отключен для аккаунта в случае если приложение для него вызвало метод [logout](logout.md) или пользователь нажал &quot;Выйти&quot; в Activity [createAutoLoginIntent](create-auto-login-intent.md)

#### Return

подходящий для приложения аккаунт для автологина

## See also

passport

| | |
|---|---|
| [logout(PassportUid)](logout.md) |  |
| [createAutoLoginIntent(Context, PassportUid, PassportAutoLoginProperties)](create-auto-login-intent.md) |  |
| [com.yandex.passport.api.PassportAutoLoginProperties](../-passport-auto-login-properties/index.md) |  |

## Parameters

passport

| | |
|---|---|
| context | контекст приложения |
| properties | параметры для поиска подходящего аккаунта |

## Throws

| | |
|---|---|
| [com.yandex.passport.api.exception.PassportAutoLoginImpossibleException](../../com.yandex.passport.api.exception/-passport-auto-login-impossible-exception/index.md) | - если подходящий аккаунт не найден, либо не получилось получить получить новый токен для аккаунтов |
| [com.yandex.passport.api.exception.PassportRuntimeUnknownException](../../com.yandex.passport.api.exception/-passport-runtime-unknown-exception/index.md) |  |
