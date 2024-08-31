# Авторизация

{% note warning %}

Для работы мессенджера под токенами приложения-хоста в интеграции нужно, чтобы в Паспорте приложение имело выданный вам ранее [скоуп авторизации](https://wiki.yandex-team.ru/messenger/doc/base-integration/#skoupavtorizacii)

{% endnote %}


- Интегрируйте [Yandex Passport SDK](https://wiki.yandex-team.ru/yandexmobile/accountmanager/help/android/ng/) (aka Account Manager)

- Реализуйте `АccountProvider` - интерфейс для получения аккаунта
Оповещает об изменениях аккаунта со стороны SDK

{% cut "Пример реализации" %}

```kotlin
internal fun getDemoMessengerSdkConfiguration(context: Context) =
    MessengerSdkConfiguration(
        ...
        accountProvider = <YourProviderHere>,
        ...
    )
```

```kotlin
object : MyAccountProvider {
    override fun requestCurrent() {
        val uid = prefs.passportUid
        val environmentId = prefs.passportEnvironment
        val passportUid = if (uid != null) PassportUid.Factory.from(
            PassportEnvironment.Factory.from(environmentId), uid
        ) else null

        MessengerSdk(context).onAccountChanged(passportUid)
    }

    override fun onAccountChanged(uid: PassportUid?) {
        if (uid == null) {
            prefs.passportUid = null
        } else {
            prefs.passportUid = uid.value
            prefs.passportEnvironment = uid.environment.integer
        }
    }
}
```

{% endcut %}

При изменении аккаунта внутри приложения необходимо вызывать метод сдк:

```kotlin
MessengerSdk(context).onAccountChanged(passportUid)
```