# Запуск мессенджера

Точкой входа в мессенджер является `MessengerSdk`.
`MessengerSdk` может быть свободно создан любое количество раз в любом месте приложения, передавая в конструктор любой `android.content.Context` (Не важно Activity, Application или Service, можно также использовать `splitties.init.appCtx`).

### Дефолтный запуск
Открывает список чатов без сторонних сайд-эффектов, логируются стартовые метрики

```kotlin
MessengerSdk(context).openMessenger(
    source = Source.forHost("a string for logging purpose")б
)
```

### Открытие экрана списка чатов

```kotlin
MessengerSdk(context).performAction(
    source = Source.forHost("a string for logging purpose"),
    action = MessagingAction.OpenChatList,
)
```

### Открытие чата
Для идентификации какой чат открыть передайте объект `ChatRequest` (С описанием различных типов чатов можно ознакомиться [тут](TODO)).

```kotlin
MessengerSdk(context).performAction(
    source = Source.forHost("a string for logging purpose"),
    action = MessagingAction.OpenChat(
        chatRequest = ChatRequests.existing(chatId)
    ),
)
```

### Изолированный чат
Помимо полного мессенджера, есть возможность встроить в приложение фрагмент с одним чатом - `MessengerChatFragment`.
Данный фрагмент представляет собой окно чата мессенджера без тулбара.

Для идентификации какой чат открыть передайте объект `ChatRequest` (С описанием различных типов чатов можно ознакомиться [тут](#shatrequests)).

Пример интеграции `MessengerChatFragment`:

```kotlin
val fragment = MessengerSdk(context).createIsolatedChatFragment(
    source = Source.forHost("a string for logging purpose"),
    chatRequest = ChatRequests.existing(chatId),
)
supportFragmentManager.beginTransaction()
    .replace(R.id.fragment_container, fragment)
    .commit()
```
{% note warning %}

Для корректной работы sticker/emoji панели у activity должны быть window флаги `View.SYSTEM_UI_FLAG_LAYOUT_STABLE` и `View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN` + добавить обработку `windowInsets`

{% cut "Пример реализации:" %}

```kotlin
rootView.setOnApplyWindowInsetsListener { v, insets ->
    v.updateLayoutParams<ViewGroup.MarginLayoutParams> {
        leftMargin = insets.systemWindowInsetLeft
        topMargin = insets.systemWindowInsetTop
        rightMargin = insets.systemWindowInsetRight
        bottomMargin = insets.systemWindowInsetBottom
    }
    insets
}
```

{% endcut %}

{% endnote %}

