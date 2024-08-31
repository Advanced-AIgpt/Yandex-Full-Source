# Обработка клика по нотификациям

Чтобы определить способ обработки клика по нотификациям укажите `ChatNotificationAction` при конфигурации сдк


```kotlin
internal fun getDemoMessengerSdkConfiguration(context: Context) =
    MessengerSdkConfiguration(
        ...
        chatNotificationAction = ChatNotificationAction.BuiltIn,
        ...
    )
```

Есть несколько способов обработки нотификации:

#### `ChatNotificationAction.BuiltIn` 
Встроенный в сдк обработчик, который откроет `MessengerActivity`

#### `ChatNotificationAction.ActionIntent`
Будет вызван `implicit activity intent` с `ChatNotificationAction.ActionIntent.ACTION` 

Добавьте `intent-filter` для своей активити, чтобы обработать событие:


```kotlin
 <intent-filter>
      <action android:name="com.yandex.messenger.Notification.ACTION"/>
 </intent-filter>
```
Запустите мессенджер:

```kotlin
override fun onCreate(savedInstanceState: Bundle?) {
    MessengerSdk(this).performAction(
        action = ChatNotificationAction.ActionIntent.getAction(intent),
        source = Source.Intent
    )
}
```

#### `ChatNotificationAction.CustomIntent`
При клике на нотификации вы сможете сформировать intent запускаемой activity

```kotlin
ChatNotificationAction.CustomIntent { chatId, chatName -> 
    Intent(context, YourCustomActivity::class.java)
}
```


#### `ChatNotificationAction.ClickHandler`
Будет вызван коллбек для обработка клика по нотификации

```kotlin
ChatNotificationAction.ClickHandler { chatId, chatName ->
    //do what you want
}
```


#### `ChatNotificationAction.Legacy`
Deprecated