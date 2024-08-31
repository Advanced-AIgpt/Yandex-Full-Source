# Введение

Админка это сервис, с помощью которого можно управлять разными сущностями - заказами, партнерами, исполнителями, заказчиками, промокодами и тд. Админкой пользуются операторы колл-центра, маркетологи для создания промокодов, контент-менеджеры для конфигурирования форм создания заказов, менеджеры по работе с партнерами и тд.

Благодаря админке повышается качество сервиса - в ней можно заблокировать плохих исполнителей, партнеров и заказчиков, можно отредактировать или отменить заказ,послушать звонки операторов КЦ к заказчикам и многое другое.

# Команда {#command}

В основном админку разрабатывает 3 команды:
* Служба Контроля Качества (СКК) - разрабатывают инструменты для блокировки, арбитража, проверки паспортов и тд
* Центр Управления Полетов (ЦУП) - работают на заказами сервисной модели и все, что с ними связано, а также над промокодами
* Клиентская команда - в админке в основном поддерживают все, что связано с заведением форм создания заказов и лендингов

Распределение по командам можно посмотреть на [странице распределения](https://wiki.yandex-team.ru/ydo/frontend/raspredelenie-ljudejj-po-komandam/)


# Доступы {#roles}

В админке для многих действий нужны определенные доступы. Доступы для определенной функциональности лучше узнавать у ответственных в команде, которая отвечает за функциональность
Если все же непонятно куда идти, можно написать [@lakate](https://staff.yandex-team.ru/lakate)

Если из админки переходить в клиентское приложение, то нужно быть залогиненым под оператором КЦ. Для тестинга можно использовать готовый аккаунт оператора, посмотреть его можно на [странице тестирования](https://wiki.yandex-team.ru/users/klimovaksyu/testirovanie-uslug)

Или можно запросить права оператора для своега аккаунта:
`Админка Услуг —> КЦ —> Оператор`
IDM предложит создать акк оператора (yndx-логин-operator), потом придет письмо с просьбой дозалогина аккаунта и можно пользоваться))

Особые доступы, даже на просмотр, нужны для страниц Рубрикатора/Фиченатора/Группинатора/Атрибутутора/Форминатора/Батчинатора: нужно состоять в [группе](https://abc.yandex-team.ru/services/uslugi_rubricator_admins/). Добавляйтесь в нее сами, а потом пишите кому-то из помеченных звездочкой, чтобы подтвердили вас.

## Стенды {#stands}

* [Прод](https://ydo-admin.yandex-team.ru/uslugi-admin/)
* [Тестинг](https://ydo-admin-dev.yandex-team.ru/uslugi-admin/) - тестинг (тестовая БД), можно использовать для регресса релиза, а также он является основным стендом для работы в рубрикаторе/фиченаторе/...
* [Мастер](https://renderer-ydo-admin-master.admin-dev.ydo.yandex-team.ru/uslugi-admin/) - бета с кодом из мастера (обновление на каждый мерж ПР)
* [Локальная бета](https://<your_login>-1-ws.tunneler-si.yandex-team.ru/uslugi-admin/) - тут важно быть на yandex-team, и в конце обязателен "/"


{% note info "Важно!" %}

Для работы в `Рубрикаторе/Фиченаторе/Группинаторе/Атрибутаторе/Форминаторе/Батчинаторе` используется именно тестовый стенд (dev/shared/rendrerer/tunneler) и ни в коем случае не продовый. Сделано это чтобы тестировать правки оперативно на тестовом стенде. Эти правки доезжают до прода на следующий день или по запросу к бекенду.

{% endnote %}
