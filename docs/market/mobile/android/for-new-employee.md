# Поздравляю, ты стал Android разработчиком в команде Яндекс.Маркет!

## 1. Необходимое ПО

* Android Studio - ide в которой мы пишем код
* Telegram - основной инструмент коммуникации с командой
* Charles - инструмент для отладки сетевых запросов
* Плагин для студии [detekt](https://plugins.jetbrains.com/plugin/10761-detekt)

## 2. Дополнительное ПО
* Figma - инструмент для работы с макетами ([доступ тут](https://wiki.yandex-team.ru/figma/))
* Postman - инструмент для тестирования энпоинтов
* SublimeText - для удобной работы с текстовыми файлами
* Slack - некоторые команды в Яндексе работают в Slack
* Intellij IDEA - пригодится ([доступ тут](https://wiki.yandex-team.ru/jetbrains/LicenseServers/))
* Git клиент (например SourceTree) - если не устраивает встроенный в Android Studio плагин
* Плагин для работы с диплинками ([скачивать здесь](https://github.com/masteroffreedom/DeeplinkHelperPlugin))
* AppMetrica - инструмент для работы с метриками приложения (id приложения=1389598) ([доступ тут](https://wiki.yandex-team.ru/yandexmobile/appmetrica/akkaunty-i-dostupy/menedzherskijj-dostup-v-appmetrica/#dostuptolkokodnomuprilozhenijujandeksa))

{% note alert %}

Доступ к AppMetrica запрашивать **"Просмотр"**

{% endnote %}
* Для **отправки пушей** нужно запросить доступ в AppMetrica с id тестового приложения `id=`2780002 (Маркет Тест) и с возможностью редактирования `role=edit`. Инструкция по работе с пушами ([здесь](https://wiki.yandex-team.ru/users/manvelova/otpravka-pushejj-dlja-mp/))

## 3. Твоя страница на стаффе

* Укажи логин в телеграмме

{% note warning %}

Укажи логин, как рабочий. Иначе код-ревью бот не будет тебе писать

{% endnote %}
* Укажи место, где ты сидишь
* Привяжи почту на yandex.ru (не yandex-team.ru)
* Создай ssh-ключ для доступа к внутрисетевым ресурсам. Подробнее [тут](https://wiki.yandex-team.ru/diy/macos/ssh/) или [тут](https://wiki.yandex-team.ru/security/ssh/macos/?from=%2Fdoc-and-loc%2Fdoc%2Fnewbies%2Fmac%2Fssh-authentication-keys%2F)
* На своей странице ты можешь найти своего руководителя, информацию о подразделении и тд

## 4. Настройка проекта

* Скачай проект из репозитория [https://arcanum.yandex-team.ru/arc_vcs/mobile/market/android/app-market](https://arcanum.yandex-team.ru/arc_vcs/mobile/market/android/app-market)
* Установи настройки [https://wiki.yandex-team.ru/users/glotovss/androidmarketcodestyle/.files/settings-2.jar](https://wiki.yandex-team.ru/users/glotovss/androidmarketcodestyle/.files/settings-2.jar) (в Android studio File->Import Settings)
* Включи git хуки: введи в терминале `git config core.hooksPath .githooks`
* Напиши боту [@codereview_bot](https://t.me/codereview_bot) в телеграмме `/start`
* Напиши боту [@TSUMYandexBot](https://t.me/TSUMYandexBot) в телеграмме `/start`
* По желанию настрой Issue navigation в Android studio: Preferences -> Version Control -> Issue navigation; нажимаешь на плюсик (Add), в Issue ID вводишь `(?i)BLUEMARKETAPPS(?-i)-\d+`, в Issue link вводишь `https://st.yandex-team.ru/$0`

## 5. Чаты и встречи

Обязательно попроси ментора добавить тебя во все необходимые чатики и встречи.

Основные чаты:
* android dev (Yandex.Market) - только android разработчики нашей команды
* Мобильная разработка и QA Маркета - разработка ios, разработка android, тестировщики
* Релизы мобилок Маркета - рабочий чат по выпуску релизов ios/android
* MarketMobile - общий чат-приемная мобилок

Встречи:
* Назначаются в [календарь](https://calendar.yandex-team.ru/)
* По понедельникам встречаемся командой Android разработчиков. Обсуждаем свои задачи на неделю, вопросы по проекту, холиварим и договариваемся о каких-то вещах.
* Подпишись на рассылку [https://ml.yandex-team.ru/lists/market-mobile-dev/](https://ml.yandex-team.ru/lists/market-mobile-dev/)

## 6. Как пишется код (устаревшая информация)

* [https://wiki.yandex-team.ru/DIY/](https://wiki.yandex-team.ru/DIY/)
* [https://wiki.yandex-team.ru/users/glotovss/androidmarketcodestyle/](https://wiki.yandex-team.ru/users/glotovss/androidmarketcodestyle/)
* [https://wiki.yandex-team.ru/users/bejibx/Stil-koda-Kotlin/](https://wiki.yandex-team.ru/users/bejibx/Stil-koda-Kotlin/)
* [https://wiki.yandex-team.ru/Market/mobile/marketapps/android/](https://wiki.yandex-team.ru/Market/mobile/marketapps/android/)
* [https://wiki.yandex-team.ru/users/apopsuenko/architecture](https://wiki.yandex-team.ru/users/apopsuenko/architecture)
* [https://wiki.yandex-team.ru/users/apopsuenko/packagesstructure](https://wiki.yandex-team.ru/users/apopsuenko/packagesstructure)
* [https://wiki.yandex-team.ru/users/apopsuenko/mainframer/](https://wiki.yandex-team.ru/users/apopsuenko/mainframer/)
* [https://wiki.yandex-team.ru/users/apopsuenko/problems/](https://wiki.yandex-team.ru/users/apopsuenko/problems/)
* [https://wiki.yandex-team.ru/users/dmpolyakov/byebye-autovalue/](https://wiki.yandex-team.ru/users/dmpolyakov/byebye-autovalue/)

## 7. Изучение проекта

Первое время могут быть сложности с навигацией по коду. Для решения этой проблемы поможет утилита [https://github.com/acelost/Spectrum](https://github.com/acelost/Spectrum).

## 8. Почитай о терминологии Маркета

* [Словарь маркета](https://wiki.yandex-team.ru/Market/frontend/development/concepts/)
* [Словарь разработчика](https://wiki.yandex-team.ru/market/mobile/marketapps/dev-dictionary/)

## 9. Девайс и внутренняя сеть

Узнай у ментора где взять смартфон для работы. Для того, чтобы через charles отслеживать трафик, тебе потребуется подключиться к сети PDAS. Выбери эту сеть, установи авторизацию TLS, сертификат login@ld.yandex.ru и введи имя пользователя login@pda-ld.yandex.ru. Если не подключается, попробуй перезагрузить ноут.

## 10. Трекер задач

Ментор или руководитель скинет тебе задачу. В трекере нажимаешь "В работу". Номер тикета используется для создания ветки.

[ссылка на оригинал](https://wiki.yandex-team.ru/market/mobile/marketapps/android/fornewworker/?from=%2Fusers%2Fapopsuenko%2Ffornewworker%2F)
