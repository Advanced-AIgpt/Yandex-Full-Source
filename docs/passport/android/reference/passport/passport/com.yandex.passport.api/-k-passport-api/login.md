//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportApi](index.md)/[login](login.md)

# login

[passport]\
abstract suspend fun [login](login.md)(activity: ComponentActivity, loginProperties: [PassportLoginProperties](../-passport-login-properties/index.md)): Result&lt;[PassportLoginResult](../-passport-login-result/index.md)&gt;

Performs login procedure in async/suspend manner. You don't need to startActivityForResult or onActivityResult-handling anymore.

Just call this function ang get profit. You will just need a ComponentActivity to do the trick.

#### Return

Result of [PassportLoginResult](../-passport-login-result/index.md)

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.KPassportIntentFactory](../-k-passport-intent-factory/create-login-intent.md) |  |
| androidx.activity.ComponentActivity |  |

## Parameters

passport

| | |
|---|---|
| activity | must be ComponentActivity variant of activity for ComponentActivity.getActivityResultRegistry usage. |
| loginProperties | a [PassportLoginProperties](../-passport-login-properties/index.md) instance. |

[passport]\
abstract suspend fun [login](login.md)(activity: ComponentActivity, loginProperties: [PassportLoginProperties.Builder](../-passport-login-properties/-builder/index.md).() -&gt; Unit): Result&lt;[PassportLoginResult](../-passport-login-result/index.md)&gt;

Performs login procedure in async/suspend manner. You don't need to startActivityForResult or onActivityResult-handling anymore.

Just call this function ang get profit. You will just need a ComponentActivity to do the trick.

#### Return

Result of [PassportLoginResult](../-passport-login-result/index.md)

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.KPassportIntentFactory](../-k-passport-intent-factory/create-login-intent.md) |  |
| androidx.activity.ComponentActivity |  |

## Parameters

passport

| | |
|---|---|
| activity | must be ComponentActivity variant of activity for ComponentActivity.getActivityResultRegistry usage. |
| loginProperties | a dsl-builder for [PassportLoginProperties](../-passport-login-properties/index.md) creation. |
