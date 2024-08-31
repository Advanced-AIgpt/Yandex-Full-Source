# SpeechKit Mobile SDK

**Целевая аудитория документа** — разработчики мобильных приложений.

**SpeechKit Mobile SDK** — мультиплатформенная библиотека, которая позволяет добавить в мобильные приложения распознавание, синтез речи и голосовую активацию.

{% note alert %}

Для того чтобы использовать библиотеку, получите API-ключ в [Кабинете разработчика](https://developer.tech.yandex.ru/). В комментарии укажите, что ключ необходим сотруднику Яндекса.

{% endnote %}

#|
|| Платформа | Способ распространения | Порядок подключения | Справочник ||
|| **Android** | Maven Artifact | 

1. Добавьте [репозиторий](http://artifactory.yandex.net/artifactory/yandex_mobile_releases/) в секцию `repositories` (файл `build.gradle` проекта):

    ```maven { url 'http://artifactory.yandex.net/artifactory/yandex_mobile_releases/' } ```

1. Добавьте зависимость в секцию `dependencies` (файл `build.gradle` приложения):

    ```xml compile'ru.yandex.android:speechkit:2.3.7' ``` | [2.3.7 (3 ноября 2015)](https://tech.yandex.ru/zout_speechkit/mobilesdk/doc/android/2.3.7/ref_911ebbf92577623f6e79a9b69c488304/concepts/About-docpage/)

{% note info %}

Порядок миграции с более ранних версий описан в [Вики](https://wiki.yandex-team.ru/users/yashrk/SpeechKitMigration/).

{% endnote %}

||
|| **iOS** | CocoaPods | 

1. Подключите [внутренний репозиторий](https://stash.desktop.dev.yandex.net/projects/MCP/repos/mobile-cocoa-pod-specs/browse/YandexSpeechKit):

    ```
    pod repo add yandex-cocoapods-stash https://stash.desktop.dev.yandex.net/scm/mcp/mobile-cocoa-pod-specs.git
    ```

    Описание установки CocoaPods приведено в [Вики](https://wiki.yandex-team.ru/yandexmobile/codebase/ios/).

1. Подключите библиотеку:
    
    ```
    source 'https://stash.desktop.dev.yandex.net/scm/mcp/mobile-cocoa-pod-specs.git'
    pod 'YandexSpeechKit', '~> 2.3.7' 
    ```

    Порядок подключения библиотеки описан в [Вики](https://old.wiki.yandex-team.ru/users/a-kononova/yandexspeechkit-ios-sdk-tutorial). | [2.3.7 (3 ноября 2015)](https://tech.yandex.ru/zout_speechkit/mobilesdk/doc/ios/2.3.7/ref_681e96f16c9e5f50ee4a1fafffe07d41/concepts/About-docpage/)

{% note info %}

Порядок миграции с более ранних версий описан в [Вики](https://wiki.yandex-team.ru/users/yashrk/SpeechKitMigration/).

{% endnote %}

||
|| **Windows Phone** | Исходный код библиотеки | Клонируйте [репозиторий](https://stash.desktop.dev.yandex.net/projects/VOICE/repos/libspeechkit/browse?at=refs%2Fheads%2Ffeature-VOICE-1318-wp81). | [2.3.7 (3 ноября 2015)](https://tech.yandex.ru/zout_speechkit/mobilesdk/doc/winphone/2.3.7/ref_ae00a161210801c58c4f0aa09cbe0916/concepts/About-docpage/)
||
|#

Общая информация о речевых технологиях Яндекса приведена в [Каталоге технологий](https://tech.yandex.ru/speechkit/).

