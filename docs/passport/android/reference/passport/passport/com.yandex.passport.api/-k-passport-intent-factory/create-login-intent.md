//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportIntentFactory](index.md)/[createLoginIntent](create-login-intent.md)

# createLoginIntent

[passport]\
abstract fun [createLoginIntent](create-login-intent.md)(context: Context, loginProperties: [PassportLoginProperties](../-passport-login-properties/index.md)): Intent

Создание Intent-а, использование которого в startActivityForResult() откроет окно выбора аккаунта или же регистрации нового аккаунта.

#### Return

Intent для показа окна АМ.

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.PassportLoginProperties](../-passport-login-properties/index.md) |  |
| android.app.Activity |  |

## Parameters

passport

| | |
|---|---|
| context | Контекст приложения. |
| loginProperties | Параметры, влияющие на вид и поведение открываемого окна. |
