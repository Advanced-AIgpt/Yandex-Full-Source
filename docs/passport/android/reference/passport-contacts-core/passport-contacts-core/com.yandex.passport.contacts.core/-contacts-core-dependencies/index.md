//[passport-contacts-core](../../../index.md)/[com.yandex.passport.contacts.core](../index.md)/[ContactsCoreDependencies](index.md)

# ContactsCoreDependencies

[passport-contacts-core]\
@Module

class [ContactsCoreDependencies](index.md)(val analytics: [Analytics](../-analytics/index.md), val identityProvider: [IdentityProvider](../-identity-provider/index.md), val oAuthTokenProvider: [OAuthTokenProvider](../-o-auth-token-provider/index.md), val coroutineDispatchers: CoroutineDispatchers = CoroutineDispatchersImpl())

External dependencies for contacts-core lib. Must be provided by any host using contacts-core.

## Constructors

| | |
|---|---|
| [ContactsCoreDependencies](-contacts-core-dependencies.md) | [passport-contacts-core]<br>fun [ContactsCoreDependencies](-contacts-core-dependencies.md)(analytics: [Analytics](../-analytics/index.md), identityProvider: [IdentityProvider](../-identity-provider/index.md), oAuthTokenProvider: [OAuthTokenProvider](../-o-auth-token-provider/index.md), coroutineDispatchers: CoroutineDispatchers = CoroutineDispatchersImpl()) |

## Properties

| Name | Summary |
|---|---|
| [analytics](analytics.md) | [passport-contacts-core]<br>@get:Provides<br>val [analytics](analytics.md): [Analytics](../-analytics/index.md)<br>Analytics abstraction. Use app-metrica in most cases. |
| [coroutineDispatchers](coroutine-dispatchers.md) | [passport-contacts-core]<br>@get:Provides<br>val [coroutineDispatchers](coroutine-dispatchers.md): CoroutineDispatchers<br>Can be used by host to override CoroutineDispatchers, used in suspend coroutine code. |
| [identityProvider](identity-provider.md) | [passport-contacts-core]<br>@get:Provides<br>val [identityProvider](identity-provider.md): [IdentityProvider](../-identity-provider/index.md)<br>Interface for host apps to provide installation identities. |
| [oAuthTokenProvider](o-auth-token-provider.md) | [passport-contacts-core]<br>@get:Provides<br>val [oAuthTokenProvider](o-auth-token-provider.md): [OAuthTokenProvider](../-o-auth-token-provider/index.md)<br>Interface for host apps to provide account token. |
