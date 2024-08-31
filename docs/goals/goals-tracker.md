# Работа с целями через {{ tracker-name }}

Цели хранятся в базе {{ tracker-name }} и отображаются как задачи в очереди [GOALZ](https://st.yandex-team.ru/goalz/). Параметры целей, связи, комментарии и так далее отображаются в соответствующих полях задачи. Все изменения, которые вносятся в цель через {{ tracker-name }}, отображаются в сервисе Цели и наоборот.

Это позволяет использовать для работы с целями возможности {{ tracker-name }}, например:

* Поиск целей в {{ tracker-name }} с помощью [фильтров](https://doc.yandex-team.ru/tracker/external/user/create-filter.html).
* Вывод статистики по целям на [дашборды](https://doc.yandex-team.ru/tracker/external/user/dashboard.html).
* Управление целями на [доске Agile](https://doc.yandex-team.ru/tracker/external/user/agile.html).
* Редактирование целей через API {{ tracker-full-name }}. Подробнее о работе с API читайте в документации:
    * [Справочник API {{ tracker-full-name }}.](https://docs.yandex-team.ru/cloud/tracker/about-api)
    * [Сгенерированная документация.](https://st-api.yandex-team.ru/docs/)
    
Чтобы редактировать цели через интерфейс {{ tracker-name }} или API, используйте таблицу соответствия между параметрами целей и задач.

#|
|| **Параметры цели** | **Параметры задачи в {{ tracker-full-name }}** | **Как работать через API {{ tracker-full-name }}** ||
|| ID цели (указан в URL) | ID в ключе задачи | [Получить задачу по ID](https://docs.yandex-team.ru/cloud/tracker/concepts/issues/get-issue) ||
|| Название | Название | [Редактировать задачу](https://docs.yandex-team.ru/cloud/tracker/concepts/issues/patch-issue), параметр `summary` ||
|| Описание | Описание | [Редактировать задачу](https://docs.yandex-team.ru/cloud/tracker/concepts/issues/patch-issue), параметр `description` ||
|| Ответственный | Исполнитель | [Редактировать задачу](https://docs.yandex-team.ru/cloud/tracker/concepts/issues/patch-issue), параметр `assignee` ||
|| Заказчики | Заказчики | [Редактировать задачу](https://docs.yandex-team.ru/cloud/tracker/concepts/issues/patch-issue), параметр `customers` ||
|| В главных ролях | В главных ролях | [Редактировать задачу](https://docs.yandex-team.ru/cloud/tracker/concepts/issues/patch-issue), параметр `participants` ||
|| Пользователи, подписанные на цель | Наблюдатели | [Редактировать задачу](https://docs.yandex-team.ru/cloud/tracker/concepts/issues/patch-issue), параметр `followers` ||
|| Статус | Статус | [Выполнить переход в заданный статус](https://docs.yandex-team.ru/cloud/tracker/concepts/issues/new-transition)
[Получить список доступных переходов](https://docs.yandex-team.ru/cloud/tracker/concepts/issues/get-transitions) ||
|| Важность | Важность цели | [Редактировать задачу](https://docs.yandex-team.ru/cloud/tracker/concepts/issues/patch-issue), параметр `goalimportance` ||
|| Срок | Дедлайн (по умолчанию отображается последний день квартала) | [Редактировать задачу](https://docs.yandex-team.ru/cloud/tracker/concepts/issues/patch-issue), параметр `deadline` 

Допустимое значение — строка в формате 2020-09-30 ||
|| Доступ в состоянии «закрыт» | В задаче есть компонент `is_confidential` | [Редактировать задачу](https://docs.yandex-team.ru/cloud/tracker/concepts/issues/patch-issue), параметр `components` ||
|| Категории (теги) | Теги | [Редактировать задачу](https://docs.yandex-team.ru/cloud/tracker/concepts/issues/patch-issue), параметр `tags` ||
|| Цели и задачи на вкладке **Зависимости** | Связи типа <q>Зависит от</q> | [Получить список связей](https://docs.yandex-team.ru/cloud/tracker/concepts/issues/get-links)
[Создать связь](https://docs.yandex-team.ru/cloud/tracker/concepts/issues/link-issue)
[Удалить связь](https://st-api.yandex-team.ru/docs/#operation/deleteIssueLinkApiV2)||
|| Цели на вкладке **Зависят от этой цели** | Связи типа <q>Блокирует</q> | [Получить список связей](https://docs.yandex-team.ru/cloud/tracker/concepts/issues/get-links)
[Создать связь](https://docs.yandex-team.ru/cloud/tracker/concepts/issues/link-issue)
[Удалить связь](https://st-api.yandex-team.ru/docs/#operation/deleteIssueLinkApiV2) ||
|| Метрики (графики) | Отображаются как пункты чеклиста | С метриками можно работать как с пунктами чеклиста (см. ниже). Пункт-метрика должен иметь параметры: 
* `"url": "<ссылка на график в {{ datalens-name }}>"`
* `"checklistItemType": "metric"` ||
|| Чеклист цели | Чеклист задачи | [Работать с чеклистом](https://docs.yandex-team.ru/cloud/tracker/concepts/issues/add-checklist-item) 

Пункт чеклиста цели должен иметь параметр: `"checklistItemType": "criterion"` ||
|| Комментарий к цели | Комментарий к задаче | [Получить список комментариев](https://docs.yandex-team.ru/cloud/tracker/concepts/issues/get-comments)
[Добавить комментарий](https://docs.yandex-team.ru/cloud/tracker/concepts/issues/add-comment)
[Редактировать комментарий](https://docs.yandex-team.ru/cloud/tracker/concepts/issues/edit-comment)
[Удалить комментарий](https://docs.yandex-team.ru/cloud/tracker/concepts/issues/delete-comment) ||
|#
