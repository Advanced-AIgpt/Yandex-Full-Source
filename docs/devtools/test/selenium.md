# Selenium

[Selenium](https://selenium.dev/) — это "ферма" браузеров, в которых можно выполнять произвольные действия: открывать страницы, кликать по кнопкам, вводить текст в поля, выполнять JavaScript, делать скриншоты и так далее. Взаимодействие с браузером от запуска до закрытия идет через API (т.е. из кода программы), реализующего стандарт [W3C WebDriver Protocol](https://www.w3.org/TR/webdriver/). Этот протокол является фактическим мировым стандартом автоматизации действий в браузере.

В коде тестов необходимо указывать следующий адрес (Selenium URL):

`http://login:password@sg.yandex-team.ru:4444/wd/hub`

Здесь `login` и `password` — это ваша [квота](selenium.md#quota). Если у вас ещё нет квоты, то можно использовать общую квоту:

`http://selenium:selenium@sg.yandex-team.ru:4444/wd/hub`

**Список доступных браузеров:** [https://selenium.yandex-team.ru/](https://selenium.yandex-team.ru/).

**Очередь задач Selenium:** [QAFW](https://st.yandex-team.ru/QAFW).

**Статьи про Selenium:**

* [Про Selenium: Введение](https://clubs.at.yandex-team.ru/arcadia/22488)
* [Про Selenium: Тестирование в настольных браузерах](https://clubs.at.yandex-team.ru/arcadia/22977)
* [Про Selenium: Тестирование на мобильных платформах](http://clubs.at.yandex-team.ru/arcadia/23673)

## Как использовать { #usage }

Для использования Selenium необходимо:

1. Заказать [доступы](selenium.md#firewall) от себя до Selenium и от Selenium до машин, которые хотите тестировать.
2. Определиться нужна ли вам отдельная [квота](selenium.md#quota) с браузерами и заказать ее.
3. Установить [клиентскую библиотеку](selenium.md#tools) или инструмент для работы с Selenium.
4. Новости, связанные с Selenium, отправляются в [клуб](https://clubs.at.yandex-team.ru/arcadia/). Вопросы, связанные с Selenium, можно задавать через [Единое окно поддержки](https://forms.yandex-team.ru/surveys/devtools).

## Доступы { #firewall }

Для начала работы вам необходимо проверить доступы:

* От нужных групп сотрудников на [Staff](https://staff.yandex-team.ru/) или сервисов в [ABC](https://abc.yandex-team.ru/) до `sg.http.yandex.net:4444`. Этот доступ нужен для работы с Selenium с ноутбуков.
* От виртуальных машинок разработчиков и агентов CI-систем (Jenkins, TeamCity, Sandbox) до `sg.http.yandex.net:4444`. Этот доступ нужен, если вы запускаете тесты на отдельных машинках или в CI-системах.
* От сети `_SELENIUMGRIDNETS_` (в ней запускаются браузеры) до хостов из внутренней сети, на которые будете ходить браузером.

{% note warning %}

Важно заказать доступы не только до тестируемого сервиса, но также до различных хранилищ статических файлов: аватарок, шрифтов, CSS-стилей, Javascript файлов и так далее, если эти файлы загружаются с других доменов. Если до какого-то сервера со статическими ресурсами нет доступа, то страница может зависать при загрузке, и ваши тесты будут падать с таймаутами.

{% endnote %}

Любые недостающие доступы можно заказать через [Puncher](https://puncher.yandex-team.ru/).

## Квоты { #quota }

Для доступа в Selenium вам необходимо указать **имя пользователя** (**имя квоты**) и **пароль**.

{% note info %}

Квота в Selenium - это НЕ ваши личные логин / пароль. Квота выдаётся на команду и даёт доступ к ограниченному набору браузеров. Использование выделенной квоты означает, что на ваших вычислительных мощностях запускаются только ваши браузеры и всегда доступно гарантированное количество браузеров.

{% endnote %}

Если вы только начинаете использовать Selenium, то имеется можно использовать общедоступныю квоту:

`http://selenium:selenium@sg.yandex-team.ru:4444/wd/hub`

Эта квота содержит достаточный набор свежих браузеров, подходящий для задач, требующих единиц и десятков параллельно запущенных браузеров. Если ваша задача требует большого числа браузеров (сотни и тысячи параллельно работающих браузеров), то закажите ее через [тикет](https://st.yandex-team.ru/QAFW), уточнив откуда брать вычислительные мощности ("железо"). Для просмотра имеющихся квот можно использовать [веб-интерфейс Selenium](https://selenium.yandex-team.ru/).

## Инструменты для работы с Selenium { #tools }

Инструменты можно условно поделить на 2 большие части:

1. **Клиентские библиотеки.** Оборачивают HTTP API Selenium в удобный интерфейс. Последовательность действий по работе с браузером задаёте вы.
2. **Инструменты для решения конкретных задач при помощи Selenium.** Более высокоуровневые инструменты. Решают конкретный класс задач, например, создание и сравнение скриншотов. Используют внутри себя клиентские библиотеки для доступа к браузерам.

В Яндексе Selenium код преимущественно пишется на 3 языках: [Java](https://www.java.com/), [JavaScript](https://en.wikipedia.org/wiki/JavaScript) и [Python](https://www.python.org/). Большой список популярных библиотек и инструментов для работы с Selenium можно найти [здесь](https://github.com/christian-bromann/awesome-selenium/).

## Нестандартные API { #api }

Selenium как сервис имеет некоторые служебные API, не описанные в стандарте:

URL | Описание | Пример запроса | Пример ответа
:--- | :--- | :--- | :---
GET `https://selenium.yandex-team.ru/quota/<quota>/status` | Текущее использование браузеров в конкретной квоте. **Включается явно для каждой квоты администраторами сервиса.** | `https://selenium.yandex-team.ru/quota/web4/status` | `{"MicrosoftEdge":{"12.1":{"total":3,"used":0}},"chrome":{"42.0":{"total":50,"used":0},"53.0":{"total":1825,"used":162},"65.0":{"total":1250,"used":32}},"firefox":{"46.0":{"total":690,"used":98},"57.0":{"total":50,"used":1}},"internet explorer":{"10":{"total":54,"used":0},"11":{"total":774,"used":152},"8":{"total":40,"used":0},"9":{"total":60,"used":0}},"opera":{"12.16":{"total":125,"used":0}},"safari":{"11.0":{"total":70,"used":3}}}`
GET `https://selenium.yandex-team.ru/quota/<quota>/<browser name>/<browser version>/status` | Текущее использование браузера конкретной версии в конкретной квоте. **Включается явно для каждой квоты администраторами сервиса.** | `https://selenium.yandex-team.ru/quota/web4/chrome/42.0/status` | `{"total":50,"used":0}`
GET `https://selenium.yandex-team.ru/logs/<session-id>` | Получить логи Selenium сессии по её идентификатору. Значение идентификатора можно получить в коде теста. | `https://selenium.yandex-team.ru/logs/beeb634a17091ce1f1903d322252f3` | Текст лога
GET `https://selenium.yandex-team.ru/video/<session-id>` | Получить видео Selenium сессии по ее идентификатору. Значение идентификатора можно получить в коде теста. Для записи видео требуется указать capabilities `enableVideo = true` | `https://selenium.yandex-team.ru/video/9c3349400602e8f701785adecf2ef48ee682e2e51d74599a106404d74ccbd093` | Видеозапись mp4 (на macOS - mkv) |
`wss://selenium.yandex-team.ru/vnc/<session-id>` | Получить [VNC](https://en.wikipedia.org/wiki/Virtual_Network_Computing) ([RFB](https://en.wikipedia.org/wiki/RFB_protocol)) трафик Selenium сессии по её идентификатору. Значение идентификатора можно получить в коде теста. | `wss://selenium.yandex-team.ru/vnc/beeb634a17091ce1f1903d322252f3` | Сырой VNC трафик, передаваемый через веб-сокет. Для отображения такого трафика нужно использовать клиента, например, [noVNC](https://github.com/novnc/noVNC).
