# Подключение и инициализация

{% note tip %}

Прежде чем интегрировать SDK, рекомендуем прочитать [общую документацию](https://wiki.yandex-team.ru/messenger/doc/).

{% endnote %}

## Подключение SDK

### Подготовка
  Java 1.8 compatibility


```groovy
kotlinOptions {
    jvmTarget = '1.8'
}

compileOptions {
    sourceCompatibility JavaVersion.VERSION_1_8
    targetCompatibility JavaVersion.VERSION_1_8
}
```


### Подключение artifactory
Добавьте yandex mobile репозиторий в ваш project-level Gradle файл (build.gradle)

```groovy
repositories {
    maven { url 'http://artifactory.yandex.net/artifactory/yandex_mobile_releases/' }
}
```
### Подключение полного SDK
В случае если вам нужен полный SDK мессенджера со всеми доступными возможностями, то просто добавьте messaging зависимость в ваш app-level Gradle файл (build.gradle)

```groovy
dependencies {
  implementation "com.yandex.messenger:messaging:{alicekitVersion}"
}
```
### Подключение модульного SDK
В случае если для вашего приложения критичен размер артефакта вы можете использовать модульное подключение и подключать только ту функциональность которая реально нужна.

Для начала подключается базовое ядро SDK:

```groovy
dependencies {
  implementation "com.yandex.messenger:messaging-core:{alicekitVersion}"
}
```
Если вы хотите расширить функциональность мессенджера дополнительными возможностями, то подключите дополнительно один или несколько модулей:

```groovy
dependencies {
  implementation "com.yandex.messenger:messaging-onboarding:{alicekitVersion}" // Онбординг
  implementation "com.yandex.messenger:messaging-shortcut:{alicekitVersion}" // Вынос иконки на рабочий стол
  implementation "com.yandex.messenger:messaging-voice-input:{alicekitVersion}" // Запись голосовых сообщений
  implementation "com.yandex.messenger:messaging-div:{alicekitVersion}" // Поддержка дивных карточек в сообщениях
  implementation "com.yandex.messenger:messaging-media-video:{alicekitVersion}" // Видеоплеер
  implementation "com.yandex.messenger:messaging-media-audio:{alicekitVersion}" // Аудиоплеер (для голосовых сообщений)
  implementation "com.yandex.messenger:messaging-attachments:{alicekitVersion}" // Загрузчик фото/видео файлов
}
```

## Инициализация SDK

Создайте класс `com.yandex.messaging.sdk.MessengerHost` унаследовав от `MessengerHostSpec`.
В нем необходимо будет реализовать интерфейс передав конфигурацию для SDK и имя хоста. У класса должен быть конструктор принимающий единственный аргумент `android.content.Context`.
SDK самостоятельно создаст экземпляр данного класса.

{% cut "Пример реализации:" %}

```kotlin
package com.yandex.messaging.sdk

class MessengerHost(private val context: Context) : MessengerHostSpec {

    //здесь нужно указать имя вашего приложения (будет использоваться при логировании)
    override val hostName = "<YourHostName>" 

    override val configuration: MessengerSdkConfiguration
        get() = MessengerSdkConfiguration(context)
}
```

{% endcut %}