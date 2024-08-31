//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[UserCredentials](index.md)

# UserCredentials

[passport]\
data class [UserCredentials](index.md)(environment: Environment, login: String, password: String, avatarUrl: String? = null) : Parcelable, [PassportUserCredentials](../-passport-user-credentials/index.md)

## Constructors

| | |
|---|---|
| [UserCredentials](-user-credentials.md) | [passport]<br>fun [UserCredentials](-user-credentials.md)(environment: Environment, login: String, password: String, avatarUrl: String? = null) |

## Types

| Name | Summary |
|---|---|
| [Companion](-companion/index.md) | [passport]<br>object [Companion](-companion/index.md) |

## Functions

| Name | Summary |
|---|---|
| [getAvatarUrl](get-avatar-url.md) | [passport]<br>open override fun [getAvatarUrl](get-avatar-url.md)(): String? |
| [getEnvironment](get-environment.md) | [passport]<br>open override fun [getEnvironment](get-environment.md)(): Environment |
| [getLogin](get-login.md) | [passport]<br>open override fun [getLogin](get-login.md)(): String |
| [getPassword](get-password.md) | [passport]<br>open override fun [getPassword](get-password.md)(): String |
