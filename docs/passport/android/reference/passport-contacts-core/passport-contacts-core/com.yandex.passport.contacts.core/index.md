//[passport-contacts-core](../../index.md)/[com.yandex.passport.contacts.core](index.md)

# Package com.yandex.passport.contacts.core

## Types

| Name | Summary |
|---|---|
| [Analytics](-analytics/index.md) | [passport-contacts-core]<br>@AnyThread<br>interface [Analytics](-analytics/index.md)<br>Interface, used to report some analytics events. Push them to Metrica if you want. |
| [ContactPermissionFacade](-contact-permission-facade/index.md) | [passport-contacts-core]<br>class [ContactPermissionFacade](-contact-permission-facade/index.md)@Injectconstructor(context: Context, permissionManager: PermissionManager, coroutineDispatchers: CoroutineDispatchers)<br>Facade class designed to check contact permissions before actual contacts sync request. |
| [ContactPermissionRationale](-contact-permission-rationale/index.md) | [passport-contacts-core]<br>interface [ContactPermissionRationale](-contact-permission-rationale/index.md)<br>Configuration for rationale to show before requesting a permission. |
| [ContactsCoreComponent](-contacts-core-component/index.md) | [passport-contacts-core]<br>@Singleton<br>@Component(modules = [[ContactsCoreModule::class](../../passport-contacts-core/com.yandex.passport.contacts.core/index.md), [ContactsCoreDependencies::class](-contacts-core-dependencies/index.md)])<br>interface [ContactsCoreComponent](-contacts-core-component/index.md)<br>Core dagger2 component for contacts lib core. Use [Builder](-contacts-core-component/-builder/index.md) to create. [ContactsCoreDependencies](-contacts-core-dependencies/index.md) and application Context are required. |
| [ContactsCoreDependencies](-contacts-core-dependencies/index.md) | [passport-contacts-core]<br>@Module<br>class [ContactsCoreDependencies](-contacts-core-dependencies/index.md)(val analytics: [Analytics](-analytics/index.md), val identityProvider: [IdentityProvider](-identity-provider/index.md), val oAuthTokenProvider: [OAuthTokenProvider](-o-auth-token-provider/index.md), val coroutineDispatchers: CoroutineDispatchers = CoroutineDispatchersImpl())<br>External dependencies for contacts-core lib. Must be provided by any host using contacts-core. |
| [ContactsSyncResult](-contacts-sync-result/index.md) | [passport-contacts-core]<br>data class [ContactsSyncResult](-contacts-sync-result/index.md)(val uploadedCount: Int, val deletedCount: Int)<br>Upload statistics as a result of contacts sync iteration. |
| [ContactSyncFacade](-contact-sync-facade/index.md) | [passport-contacts-core]<br>class [ContactSyncFacade](-contact-sync-facade/index.md)<br>Facade class for actually synchronizing contacts. |
| [IdentityProvider](-identity-provider/index.md) | [passport-contacts-core]<br>interface [IdentityProvider](-identity-provider/index.md)<br>Interface for host apps to provide installation identities. |
| [OAuthTokenProvider](-o-auth-token-provider/index.md) | [passport-contacts-core]<br>interface [OAuthTokenProvider](-o-auth-token-provider/index.md)<br>Interface for host apps to provide account token. |

## Functions

| Name | Summary |
|---|---|
| [ContactsCoreComponent](-contacts-core-component.md) | [passport-contacts-core]<br>inline fun [ContactsCoreComponent](-contacts-core-component.md)(init: [ContactsCoreComponent.Builder](-contacts-core-component/-builder/index.md).() -&gt; Unit): [ContactsCoreComponent](-contacts-core-component/index.md)<br>Creates ContactsCoreComponent in a more Kotlin-friendly manner. |
