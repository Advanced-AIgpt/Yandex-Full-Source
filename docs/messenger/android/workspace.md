# Изоляция по workspaceId

Если вам был [выдан workspaceId](https://wiki.yandex-team.ru/messenger/doc/base-integration/#vorkspejjs), укажите его при конфигурации сдк мессенджера:

```kotlin
internal fun getDemoMessengerSdkConfiguration(context: Context) =
    MessengerSdkConfiguration(
        ...
        messagingConfiguration = MessagingConfiguration(
                workspaceId = <Your workspaceId>,
                ...
            ),
        ...
    )
```