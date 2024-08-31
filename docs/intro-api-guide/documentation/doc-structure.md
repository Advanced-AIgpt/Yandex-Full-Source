# Как задокументировать API

## Перед документированием {#test}

Если в документации есть раздел **Быстрый старт** или аналогичный, выполните все шаги. Если есть песочница, зарегистрируйтесь в ней и попробуйте выполнить запрос.

Если такого раздела нет, обратитесь к заказчику. Выясните, есть ли тестовая среда.

{% cut "Инструкция для теста раздела «Быстрый старт»" %}


1. Убедитесь, что порядок действий описан верно.
1. Выпишите незнакомые термины, сокращения и другие вопросы.
1. Выполните запрос:
    - Определите, соответствует ли ответ тому, что находится в документации.
    - Протестируйте разные варианты параметров в конечной точке и проверьте, соответствуют ли ответы ожидаемым.
    
1. Определите любую отсутствующую или неточную информацию. Если нашлась неверная информация, то расскажите об этом коллегам.

{% endcut %}


## Структура типового документа с описанием API {#document-structure}

#### Обязательные разделы

#|
||**Название раздела** | **Содержание** | **Примеры** ||
||
- Введение
- Общие сведения
- <название объекта описания>
- Обзор | Общая часть про сервис и информация про API:
- что предоставляет;
- какие задачи позволяет решать;
- основные возможности. | [Введение API Яндекс&#160;Такси](https://yandex.ru/dev/taxi/doc/dg/concepts/about.html)

[Что такое API Яндекс&#160;Директа](https://yandex.ru/dev/direct/doc/start/intro.html)

[Введение API Яндекс&#160;Партнерский интерфейс](https://yandex.ru/dev/partner/doc/statistics/concepts/about.html)||
||Быстрый старт | Описание действий (нумерованный список), который приводит читателя к успеху в технологии. Например, результатом может быть ответ на тестовый запрос.

В списке приводятся ссылки на последующие разделы (получить доступ и т.п). | [Быстрый старт API Яндекс&#160;Толоки](https://yandex.ru/dev/toloka/doc/concepts/quickstart.html)

[Быстрый старт API ADFOX](https://yandex.ru/dev/adfox/doc/v.1/concepts/description.html)||
||Подключение и доступ | Может содержать описания:
- как отправить заявку на получение доступа к API;
- как получить OAuth-токен;
- как включить Песочницу.

Допустимо разбить на отдельные страницы. | [Авторизация API Яндекс&#160;Метрики](https://yandex.ru/dev/metrika/doc/api2/intro/authorization.html)

[Доступ и авторизация API Яндекс&#160;Директа](https://yandex.ru/dev/direct/doc/dg/concepts/access.html)

[Доступ к API Видеохостинга](https://yandex.ru/dev/videohosting/doc/concepts/access.html)

[Доступ к API Яндекс&#160;Партнерский интерфейс](https://yandex.ru/dev/partner/doc/statistics/concepts/access.html)

[Доступ к API ADFOX](https://yandex.ru/dev/adfox/doc/v.1/concepts/access.html) ||
||
- Ограничения
- Лимиты
- Квоты | Информация об ограничениях. | [Квотирование API Яндекс&#160;Аудиторий](https://yandex.ru/dev/audience/doc/intro/quotas.html)

[Ограничения для операций с ресурсами API Яндекс&#160;Партнерский интерфейс](https://yandex.ru/dev/partner/doc/objects/concepts/restrictions.html)||
||Справочник | Описание возможностей этого API. | Возможна группировка ручек:
- По ресурсам:
    - [Справочник API Яндекс.Cloud](https://cloud.yandex.ru/docs/iam/api-ref/Federation/)
    - [Справочник API Яндекс&#160;Директа](https://yandex.ru/dev/direct/doc/ref-v5/concepts/about.html)
    - [Операции с ресурсами API Видеохостинга](https://yandex.ru/dev/videohosting/doc/reference-v2/all-resources.html)
    
- Хаотичная ([AppMetrica](https://tech.yandex.ru/appmetrica/doc/mobile-api/push/all-resources-docpage/)).||
|#

#### Рекомендованные разделы

#|
||Название раздела | Содержание | Примеры||
||Сценарии использования | Порядок действий для решения популярных задач. | [Пошаговые инструкции API Яндекс.Cloud](https://cloud.yandex.ru/docs/iam/operations/)

[Примеры сценариев API Яндекс&#160;Партнерский интерфейс](https://yandex.ru/dev/partner/doc/objects/concepts/examples.html)||
||- Версии
- История изменений | Информация о добавленных, обновленных и удаленных функциональностях. | [ClickHouse release notes](https://clickhouse.tech/docs/ru/whats-new/changelog/)

[История изменений API Яндекс&#160;Директа версии 5](https://yandex.ru/dev/direct/doc/changelog/index.html)||
|#

#### Опциональные разделы

#|
||Название раздела | Содержание | Примеры||
||Руководство | 
- Рекомендации по использованию.
- Инструкции по выполнению атомарных процедур. | [Руководство разработчика YandexAudio API](https://yandex.ru/dev/audio/jsapi/doc/dg/concepts/about.html)

[Руководство разработчика API Яндекс&#160;Директа](https://yandex.ru/dev/direct/doc/dg/concepts/about.html)||
||Глоссарий | Термины и определения. | [Глоссарий API Яндекс&#160;Виджетов](https://yandex.ru/dev/wdgt/doc/dg/appendices/glossary.html?lang=ru)

[Список терминов API Яндекс&#160;Директа](https://yandex.ru/dev/direct/doc/dg/concepts/glossary.html)||
||Руководство по переходу | 
- Информация об актуальной и архивных версиях API.
- Порядок действий для перехода к определенной версии. | [Переход к API Яндекс ID от OpenID](https://yandex.ru/dev/id/doc/dg/concepts/openid-migrate.html)

[Переход к API Яндекс&#160;Директа версии 5 от версии 4](https://yandex.ru/dev/direct/doc/migration/concepts/about.html)||
||Примеры | Примеры запросов и ответов. | [Пример настройки дополнения API Блокировки контента Яндекс&#160;Браузера](https://yandex.ru/dev/browser/contentblocker/doc/examples/index.html)

[Примеры запросов к API Яндекс&#160;Директа](https://yandex.ru/dev/direct/doc/examples-v5/all-examples.html)||
||Решение проблем | Частые вопросы и ответы на них. | [Решение проблем API Яндекс&#160;Переводчика](https://yandex.ru/dev/translate/doc/dg/concepts/about.html)||
||Теоретические сведения | Дополнительная информация о работе сервиса. | [Теоретические сведения API Яндекс&#160;Карт](https://yandex.ru/dev/maps/jsapi/doc/2.1/theory/index.html)||
|#

## Структура раздела {#page-structure}

Стандартная структура страницы с описанием отдельного метода API содержит:

- Краткое описание ресурса и задачи, которую он выполняет.
- Формат запроса.
- Тело запроса.
- Параметры запроса.
- Формат ответа.
- Параметры ответа.
- Пример запроса и ответа.

## Значения параметров {#values}

Указывайте в параметрах максимальные, минимальные и допустимые значения. Например, если API допускает передачу даты и времени в формате ISO 8601, то эти ограничения нужно прописать.

## Типы данных {#data}

- `string` — строка, последовательность букв и/или цифр;
- `integer` — целое число (положительное или отрицательное);
- `boolean` — логическое значение `true` или `false`;
- `object` — объект, пара ключ-значение в формате JSON;
- `array` — массив значений.

## Узнайте больше {#learn-more}
* [Как документировать API на английском](https://english-style-guide.daas.yandex-team.ru/api-and-sdk/index.html)