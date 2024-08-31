# Добро пожаловать в Яндекс.Маркет!

Поздравляю, ты прошел/прошла все собеседования Маркета, чтобы оказаться в команде. Данная статья поможет быстро влиться в процесс разработки и подготовится к работе.

## Программы

### Среда разработки

Для начала надо установить среду разработки в зависимости от команды:

{% list tabs %}

- Android

  * Android Studio — среда разработки (IDE)
  * Плагин для студии [detekt](https://plugins.jetbrains.com/plugin/10761-detekt)
  * Плагин для работы с диплинками ([скачивать здесь](https://arcanum.yandex-team.ru/arc_vcs/mobile/market/tools/android-market-plugin))
  * Утилита [Spectrum](https://github.com/acelost/Spectrum) для навигации по коду

- iOS

  [XCode](https://apps.apple.com/ru/app/xcode/id497799835) — среда разработки (IDE)

{% endlist %}

### Основное ПО

Затем есть несколько программ без которых нельзя будет обойтись в работе:

* [Telegram](https://desktop.telegram.org) — основной инструмент коммуникации с командой
* [Slack](http://Slack.com/) — иногда используется вместо Telegram
* [Figma](http://figma.com/) — инструмент для работы с дизайн макетами ([Доступ тут](https://wiki.yandex-team.ru/figma/))
* [Zoom](https://zoom.us) — все созвоны проходят через него

{% note tip %}

Большинство программ можно установить с помощью Self Service от Yandex, который должен быть уже на компьютере

{% endnote %}

### Дополнительное ПО

Без следующих программ можно обойтись, но они делают процесс разработки комфортнее:

* Charles/[Proxyman](https://proxyman.io) — инструмент для отладки сетевых запросов
* Postman — инструмент для тестирования сетевых запросов
* SublimeText/[Atom](https://atom.io) — для удобной работы с текстовыми файлами
* SourceTree/[Fork](https://fork.dev) — git клиент, если не устраивает встроенный в IDE плагин
* Intellij IDEA — Еще одна IDE ([Доступ тут](https://wiki.yandex-team.ru/jetbrains/LicenseServers/))

## AppMetrica

Надо получить доступ в AppMetrica — это инструмент для работы с метриками приложения. Используется во время дежурства или отладки багов в приложении. Получить доступ можно через [IDM](https://idm.yandex-team.ru/). [Подробнее](https://wiki.yandex-team.ru/yandexmobile/appmetrica/akkaunty-i-dostupy/menedzherskijj-dostup-v-appmetrica/#dostuptolkokodnomuprilozhenijujandeksa)

Доступ придется запрашивать для двух приложений и для этого понадобятся два идентификатора:
* 1389598 — идентификатор приложения Маркет
* 2780002 — идентификатор приложения Маркет Тест

{% note alert %}

Доступ к AppMetrica запрашивать **"Просмотр"**

{% endnote %}

{% note warning %}

Для **отправки пушей** нужно запросить доступ в AppMetrica с id тестового приложения (`id=`2780002 — Маркет Тест) с возможностью редактирования `role=edit`. Инструкция по работе с пушами [здесь](https://wiki.yandex-team.ru/users/manvelova/otpravka-pushejj-dlja-mp/)

{% endnote %}

## Страница на стаффе

[Стафф](https://staff.yandex-team.ru) — страница в Яндексе с основной информацией о тебе. На своей странице ты можешь найти своего руководителя, посмотреть свою з/п или просто зависнуть в очивках коллеги. Советую посидеть посмотреть, что там есть.

На странице Стафф для работы надо заполнить некоторые поля:
* Логин в телеграмме
* Рабочее место, где ты сидишь
* Почта на yandex.ru (не yandex-team.ru)
* ssh-ключ для доступа к внутрисетевым ресурсам. Подробнее, как это сделать [тут](https://wiki.yandex-team.ru/diy/macos/ssh/) и [тут](https://wiki.yandex-team.ru/security/ssh/macos/?from=%2Fdoc-and-loc%2Fdoc%2Fnewbies%2Fmac%2Fssh-authentication-keys%2F)

{% note warning %}

Логин тегерамм следует указать как рабочий, иначе код-ревью бот не будет тебе писать.

{% endnote %}

## Работа с аркадией

* Установить аркадию по [документации](https://docs.yandex-team.ru/devtools/intro/quick-start-guide)
* Установить [плагин](https://docs.yandex-team.ru/devtools/intro/quick-start-guide#ide-plugin-setup), если работаешь в Android Studio или Intellij IDEA
* [Ознакомиться](https://docs.yandex-team.ru/devtools/src/arc/workflow) с тем, как работать в аркадии

## Настройка проекта

После добавления ssh ключа на Стафф, можно приступить к настройке проекта:

{% list tabs %}

- Android

    * Скачать проект из [репозитория](https://arcanum.yandex-team.ru/arc_vcs/mobile/market/android/app-market)
    * Установить настройки по [ссылке](https://wiki.yandex-team.ru/users/glotovss/androidmarketcodestyle/.files/settings-2.jar) (в Android studio File->Import Settings)
    * Включить arc хуки, для этого необходимо создать символическую ссылку или скопировать конфиг из папки arc-hooks проекта в домашнюю директорию под именем `.arcconfig`

        `ln -s ARCADIA_PATH/mobile/market/android/app-market/arc-hooks/arcconfig ~/.arcconfig` 
        
        или 
        
        `cp ARCADIA_PATH/mobile/market/android/app-market/arc-hooks/arcconfig ~/.arcconfig`
    * По желанию настроить Issue navigation в Android studio: Preferences -> Version Control -> Issue navigation; нажать на плюсик (Add), в Issue ID ввести `(?i)BLUEMARKETAPPS(?-i)-\d+`, в Issue link ввести `https://st.yandex-team.ru/$0`

- iOS

    * Скачать проект из [репозитория](https://arcanum.yandex-team.ru/arc_vcs/mobile/market/ios/app-market)
    * Установить Command Line Tools 12.5.1
    * Следовать инструкции, которая идет в файле README.md проекта.

{% endlist %}

## Чаты и боты

После добавления рабочей телеги на Стаффе, следует попросить ментора добавить себя во все необходимые чатики и встречи твоего контура.

### Чаты
Есть общие чаты для всех контуров:
* Мобильная разработка и QA Маркета — разработка ios, разработка android, тестировщики
* Релизы мобилок Маркета — рабочий чат по выпуску релизов ios/android
* MarketMobile — общий чат-приемная мобилок

### Боты
Так же есть боты для помощи в работе:
* [@codereview_bot](https://t.me/codereview_bot) — бот в телеграмме, присылает уведомления связанные с ревью кода. Надо активировать через `/start`.
* [@TSUMYandexBot](https://t.me/TSUMYandexBot) — бот в телеграмме, присылает уведомления от ЦУМ'a. Надо активировать через `/start`.
* [@YandexHelpDeskbot](https://t.me/YandexHelpDeskbot) —  бот в телеграмме, служба поддержки Яндекс по настройке и получению оборудования. [Подробнее](https://help.yandex-team.ru)
* [@Angry Robot](https://t.me/joinchat/wf6O0bi4m6ZhMWMy) — канал пишет об ошибках в TeamCity

{% note tip %}

Рекомендуется еще подписаться на рассылку для всех [мобильных команд](https://ml.yandex-team.ru/lists/market-mobile-dev/)

{% endnote %}

## Встречи

* Все встречи назначаются в [календарь](https://calendar.yandex-team.ru/)
* По понедельникам встречается команда Android разработчиков
* По пятницам встречается команда iOS разработчиков

{% note tip %}

Команды постоянно проводят встречи и делятся новостями, рассказывают интересные вещи. Записи встреч можно посмотреть [здесь](https://wiki.yandex-team.ru/users/dmpolyakov/poleznosti-s-ezhenedelnojj-vstrechi/)

{% endnote %}

## Терминология Маркета

Для того чтобы понимать коллег, следует ознакомится с внутренней терминологией Маркета:
* [Словарь маркета](https://wiki.yandex-team.ru/Market/frontend/development/concepts/)
* [Словарь разработчика](https://wiki.yandex-team.ru/market/mobile/marketapps/dev-dictionary/)

## Девайс и внутренняя сеть

У ментора можно спросить, где взять смартфон для работы. Для того, чтобы через charles отслеживать трафик, тебе потребуется подключиться к сети PDAS. Выбери эту сеть, установи авторизацию TLS, сертификат login@ld.yandex.ru и введи имя пользователя login@pda-ld.yandex.ru. Если не подключается, попробуй перезагрузить ноут.

#### На этом все. Теперь можно попросить задачи от ментора или пройти необходимые курсы в [Мебиусе](https://moe.yandex-team.ru/courses/my).
