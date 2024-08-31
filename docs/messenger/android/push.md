# Получение пуш сообщений
- Зарегистрируйте сертификаты/секреты в [консоли Xiva](https://console.push.yandex-team.ru/#account) своих приложений. Мессенджер поддерживает отправку пушей с помощью [FCM](https://firebase.google.com/docs/android/setup) и [HMS](https://developer.huawei.com/consumer/en/doc/development/HMSCore-References/hmsmessageservice-0000001050173839).
Если вы уже работаете с Xiva для доставки любых других пушей, приложение может быть уже зарегистрировано

- Интегрируйте [FCM](https://firebase.google.com/docs/android/setup) и/или [HMS](https://developer.huawei.com/consumer/en/doc/development/HMSCore-References/hmsmessageservice-0000001050173839)

- Реализуйте `FirebaseMessagingService`/`HmsMessageService` делегировав методы в сдк:

{% list tabs %}

- FCM

    ```kotlin
    class FCMService : FirebaseMessagingService() {

        private val messengerSdk = MessengerSdk(this)
    
        override fun onNewToken(p0: String?) {
            super.onNewToken(p0)

            messengerSdk.onCloudTokenRefresh()
        }

        override fun onMessageReceived(message: RemoteMessage) {
            super.onMessageReceived(message)

            messengerSdk.onCloudMessageReceived(message.data)
        }
    }
    ```

- HMS

    ```kotlin
    class HMSService : HmsMessageService() {

        private val messengerSdk = MessengerSdk(this)
    
        override fun onNewToken(p0: String?) {
            super.onNewToken(p0)

            messengerSdk.onCloudTokenRefresh()
        }

        override fun onMessageReceived(message: RemoteMessage) {
            super.onMessageReceived(message)

            messengerSdk.onCloudMessageReceived(message.dataOfMap)
        }
    }
    ```

{% endlist %}


- Реализуйте `CloudMessagingProvider` - интерфейс для получения FCM/HMS токена

{% list tabs %}

- FCM

    ```kotlin
    class FcmTokenProvider() : CloudMessagingProvider {
        override fun getToken(): String? {
            return FirebaseInstanceId.getInstance().getToken("YourGcmSenderId", FirebaseMessaging.INSTANCE_ID_SCOPE)
        }
    }
    ```

- HMS

    ```kotlin
    class HmsTokenProvider(private val context: Context) : CloudMessagingProvider {

        override val tokenType: TokenType = TokenType.Hms

        @get:Throws(IOException::class)
        override val token: String?
            get() = try {
                val appId = Util.getAppId(context)
                HmsInstanceId.getInstance(context).getToken(appId, HmsMessaging.DEFAULT_TOKEN_SCOPE)
            } catch (e: ApiException) {
                throw IOException(e)
            }
    }
    ```

{% endlist %}


- Передайте реализацию в объект конфигурации SDK

```kotlin
internal fun getDemoMessengerSdkConfiguration(context: Context): MessengerSdkConfiguration {
    return MessengerSdkConfiguration(
        ...
        cloudMessagingProvider = <YourProviderHere>,
        ...
    )
}    
```
