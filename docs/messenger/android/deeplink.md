# Обработка диплинков мессенджера

В случае если вы не используете прямую обработку ссылок и интентов мессенджером, то объявите следующие `intent-filter` в `AndroidManifest.xml` для вашей Activity:

```xml
    <intent-filter android:autoVerify="true">
        <action android:name="android.intent.action.VIEW"/>
        <category android:name="android.intent.category.DEFAULT"/>
        <category android:name="android.intent.category.BROWSABLE"/>

        <data
            android:host="yandex.ru"
            android:pathPrefix="/chat"
            android:scheme="http" />
        <data
            android:host="yandex.ru"
            android:pathPrefix="/chat"
            android:scheme="https" />

        <data
            android:host="yandex.ru"
            android:pathPrefix="/messenger"
            android:scheme="http" />
        <data
            android:host="yandex.ru"
            android:pathPrefix="/messenger"
            android:scheme="https" />

        <data
            android:host="ya.ru"
            android:pathPrefix="/m/"
            android:scheme="http" />
        <data
            android:host="ya.ru"
            android:pathPrefix="/m/"
            android:scheme="https" />

    </intent-filter>

    <intent-filter>
        <action android:name="android.intent.action.VIEW"/>

        <category android:name="android.intent.category.DEFAULT"/>
        <category android:name="android.intent.category.BROWSABLE"/>

        <data android:scheme="messenger" android:host="chat"/>
        <data android:scheme="messenger" android:host="newyear"/>
    </intent-filter>
```

Полученные ссылки необходимо перенаправить в `MessengerSdk` следующим образом:

```kotlin
MessengerSdk(context).openLink(
    source = Source.forHost("a string for logging purpose"),
    uri = Uri.parse(url),
)
```