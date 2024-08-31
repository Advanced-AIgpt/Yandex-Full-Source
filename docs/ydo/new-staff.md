# Новому сотруднику Фронтенда

## Если это твой первый день в комании
{% cut "Прочитай это" %}

1. Прочитать [устав](https://wiki.yandex-team.ru/Ustaff/)
1. Настроить свой компьютер
    * Установить и настроить Thunderbird, WiFi, ... [http://wiki.yandex-team.ru/DIY](http://wiki.yandex-team.ru/DIY)
    * сгенерировать ssh-ключ для авторизации внутри сети:
      Инструкции для [MacOS](https://wiki.yandex-team.ru/diy/macos/ssh/), [Windows (+ PuTTY)](https://wiki.yandex-team.ru/diy/windows/ssh/) и [Linux](https://wiki.yandex-team.ru/diy/linux/ssh/).
      Публичный ключ нужно скопировать текстом и загрузить на [staff](https://staff.yandex-team.ru):
      [Изображение](https://jing.yandex-team.ru/files/a-koptsov/2016-06-07_12-40-53.png). После этого ключ сам скопируется на те машины, на которые ты можешь ходить по SSH. Это занимает некоторое время, но в течение дня он должен разъехаться.
1. [Словарь](https://wiki.yandex-team.ru/hr/mag/slovar/) терминов, которые вы можете услышать во всем Яндексе
1. [Список](https://i.yandex-team.ru/) инфраструктурных сервисов с описаниями

## Настройка рабочей машины
* `SANDBOX_AUTH_TOKEN` - Не хватает токенов: sandbox (переменная окружения SANDBOX_AUTH_TOKEN), берём [отсюда](https://sandbox.yandex-team.ru/oauth) и записываем в окружение
* `STATFACE_TOKEN` - [https://wiki.yandex-team.ru/search-interfaces/qyp/#oshibkiiproblemy](https://wiki.yandex-team.ru/search-interfaces/qyp/#oshibkiiproblemy)

## Настроить нужный редактор на редактирование коммитов**
Если вас пугает wim, как редактор по умолчанию для коммтов/пр для arc, то можно настроить любой другой. В `~/.arcconfig` добавляем

```bash
[core]
editor = code -w
```

## Полезные расширения для упрощения работы**
* [Работа](https://chrome.google.com/webstore/detail/editurl/oonajjdihojcjhmmalicajeepammcooi) с гет-параметрами
* React Developer Tools
* Redux DevTools

{% endcut %}

## Роли
Запросите роль разработчика интерфейсов в [ydo_frontend](https://abc.yandex-team.ru/services/ydo_frontend/) для получения необходимых доступов [тыц](https://jing.yandex-team.ru/files/apanichkina/Снимок%20экрана%202021-03-04%20в%2017.36.21.png)

## Каналы коммуникации
1. Зайти в воркспейс услуг в слаке (попросите инвайт от ментора). Полезные каналы:
    * `#frontend` - вопросы связвнные с фронтендом услуг (продуктовая часть и релизы). Тут есть все фронты, менеджеры, тестировщики
    * `#frontend-technologies` - канал, где только фронты обсуждают технические моменты и разработку
    * `#bug-reports` - обсуждаем баги, которые встречаем в проде/тестинге
    * `#backend` - канал с бекендерами, можно задавать вопросы по бекенду и искать ответственных
    * чат в-тимы - узнать у ментора свою в-тиму
1. Проверить доступы во внутренние сервисы
    * Zoom - яндексовый аккаунт
    * Стафф
1. Попросить ментора добавить себя в регулярные встречи (встречи по адаптации и коммуникация с HR из буткемпа важнее текущих встреч в команде)

## Терминология Услуг
Самые важные единицы, которыми оперируют Яндекс.Услуги:
* Профессия, Специальность и Услуга. [Как это выглядит в каталоге](https://wiki.yandex-team.ru/ydo/newstaff/.files/snimokjekrana2021-01-27v18.06.06.png) и [как это выглядит в публичном профиле исполнителя (worker)](https://wiki.yandex-team.ru/ydo/newstaff/.files/snimokjekrana2021-01-27v18.06.46.png)
* Сервисная модель (СМ) - заказы, который выполняют Яндекс услуги за счет компаний-партнеров или самозанятых испов, которые получают заказы через наш ERP и приложение исполнителя
* Пошаговая форма (ПФ) - форма создания заказа в сервисной модели

## Разработка фронтенда
Фронтенд проекты лежат [тут](https://a.yandex-team.ru/arc/trunk/arcadia/frontend). Проекты команды услуг начинаются с префикса `ydo`. Например, репа клиентского [приложения](https://a.yandex-team.ru/arc/trunk/arcadia/frontend/services/ydo).

Документация проектов монорепозитория - читай README.md во [frontend](https://a.yandex-team.ru/arc_vcs/frontend/docs/). Для проектов услуг надо смотреть текущую документацию, а также README.md в репозиториях

Мы разрабатываем в монорепозитории [Arcadia](https://wiki.yandex-team.ru/arcadia/) с помощью спец vcs [arc](https://docs.yandex-team.ru/arc/) , которая снаружи прикидывается git, но в кишках работает иначе. Так что знающим git комманды нужно просто заменить в командах git на arc
* [Настройка arc](https://wiki.yandex-team.ru/mobvteam/arc-macos/),
* [Работа с ARC](https://doc.yandex-team.ru/arc/manual/arc/start.html),
* [FAQ по Arc](https://wiki.yandex-team.ru/arcadia/arc/faq/)

Можно разрабатывать не только на своем рабочем ноуте, но и в виртуалке [QYP](https://wiki.yandex-team.ru/search-interfaces/qyp/)

### Процессы
* Про код-ревью есть отдельная [документация](processes/review.md)
* Про именование веток и задач есть отдельная [страница](processes/tickets.md)
* [Борда с задачами](https://st.yandex-team.ru/agile/board/8854). Настройка борды [скрин](https://jing.yandex-team.ru/files/apanichkina/трекер.png):
    * Выберите текущий спринт
    * Отсортируйте задачи за счет фильтров по контуру
    * В настройках отображения полей в задаче выберите спринт и сторипоинты
  Задачи для буткемперов [https://st.yandex-team.ru/issues/398528](https://st.yandex-team.ru/issues/398528)
* Макеты - Дизайн у нас рисуют в figma, для доступа к макетам используйте общий [аккаунт](https://wiki.yandex-team.ru/browser/design/figma/#figma)


### Окружения
У услуг есть 3 окружения: прод, престейбл, тестинг + беты разработчиков, которые используют дев базу. Окружение характеризуется бекендом/базой, которую используют и истоником шаблонов фронта
[Как скрещивать разные окужения](https://wiki.yandex-team.ru/ydo/backend/testingtransfer/).
Также про стенды следует читать страницы документации соответствующего проекта

{% note alert %}

Правила безопасности в дев окружении
* В дев окружении чаты (мессенджер) смотрят на прод, поэтому если пишешь в чат, то живой продовый пользователь получает эти сообщения
* Тоже самое с ugc (отзывы об исполнителе в профиле) - отзывы оставленные в деве утекут на прод

{% endnote %}

### Локальные беты
* Чаще всего запуск будет через команду `npm run start`, тогда бета запустится на `https://local.yandex.ru:3443` (если не работает этот хост попробуйте сбросить настройки DNS командой `npm run clean-archon`)
* Есть локальная публичная бета, чаще всего она нужна чтобы по быстрому показать бету менеджеру/дизайнеру и не ждать сборку в PR. Запуск через `npm run start:public`
* То, куда будут смотреть беты (в тестовую БД или в прод) задается конфигом в проектах, обычно он лежит в `.config/kotik/config.js`. По дефолту беты ходят в тестинг

### Релизный процесс
Релизы катятся каждый день. Каждое утро отводится релизная ветка, в которую попадает все, что влилось в dev. Далее релиз тестируется и затем дежурный катит релиз проекта.
Ответственные за фронтовые релизы: [@aleynikovsa](https://staff.yandex-team.ru/aleynikovsa) и [@nyakto](https://staff.yandex-team.ru/nyakto)
Если есть вопрос про релиз, или баг в проде, можно призвать дежурного в слаке через `@backend-duty`
[Инструкция дежурного](https://datalens.yandex-team.ru/ay071khskjpm5-ydo-ui-duty-dashboard?tab=6Z)

### Мониторинги и Графики с ошибками в проектах
- [https://datalens.yandex-team.ru/ay071khskjpm5-ydo-ui-duty-dashboard?state=6ff1e4bd514](https://datalens.yandex-team.ru/ay071khskjpm5-ydo-ui-duty-dashboard?state=6ff1e4bd514)

### Бекенд
* Основная [дока](backend.md)

### Разработка и тестирование
Для работы подтребуется как минимум два аккаунта. Один исполнитель, второй заказчик. У нас без привязки телефона авторизоваться нельзя, поэтому если есть только один нормер, то авторизовываемся через соцсети (фейсбук или гугл акк).
Также если **не требуется изменять персональные данные** можно воспользоваться аккаунтами тестировщиков.
По тестированию есть отдельная [дока](testing/testing.md)

Про разработку есть соответствующий для каждого проекта в данной доке:
- [YDO](ydo/index.md)
- [Админка](ydo-admin/index.md)
- [ERP](reference.md)
- [Аппка испа](reference.md)

## RR+AppHost
- Есть [дока](common-tech/report-renderer.md)

{% note info %}

Этот раздел постоянно обновляется, если вы чего-то не нашли можно подсмотреть стартовые странички других сервисов
* [https://wiki.yandex-team.ru/adv-interfaces/direct/begin/](https://wiki.yandex-team.ru/adv-interfaces/direct/begin/)
* [https://wiki.yandex-team.ru/maps/dev/ui/novice/](https://wiki.yandex-team.ru/maps/dev/ui/novice/)
* [https://wiki.yandex-team.ru/search-interfaces/quickstart/](https://wiki.yandex-team.ru/search-interfaces/quickstart/)

С вопросами можно приходить к [@apanichkina](https://staff.yandex-team.ru/apanichkina) или [@jsus](https://staff.yandex-team.ru/jsus) или [@lakate](https://staff.yandex-team.ru/lakate)

{% endnote %}
