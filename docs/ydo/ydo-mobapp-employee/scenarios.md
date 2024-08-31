# Сценарии в приложении

## Правила работы с link_key

В ответ с бека всегда приходит итем `erp_organization_employee`
- По умолчанию, если нет `link_key` - приходит текущий привязанный исп (по `puid`)
- **НЕ** авторизован в паспорте и **НЕ** зарегистрирован в ERP и есть `link_key` - подсовываем из `link_key`
- Авторизован в паспорте и **НЕ** зарегистрирован в ERP и есть `link_key` - подсовываем из `link_key`
- Авторизован и зарегистрирован в ERP (привязан puid) и есть `link_key` - действуем по умолчанию
- Авторизован и зареган в ERP как кто-то другой (напр менеджер) и есть `link_key` - игнорим `link_key` и кидаем ошибку (не отдаём `erp_organization_employee`)

## Доступные под puid методы

[pool_worker.py](https://a.yandex-team.ru/arc/trunk/arcadia/ydo/erp/server/lib/pool_worker.py?rev=r8795738#L43)

## Проверить ответ бекенда для исполнителя

Добавить параметр `json_dump_responses=YDO_ERP_BACKEND` в адресную строку
[Пример](https://shared-dev.hamster.yandex.ru/uslugi/employee/schedule?json_dump_responses=YDO_ERP_BACKEND)

## Посмотреть заказы в прошлом/будущем

Добавить параметр с нужной датой в формате `YYYY-MM-DD`в адресную строку
`exp_flags=employee_current_date%3D<date_key>` 
[Пример](https://shared-dev.hamster.yandex.ru/uslugi/employee/schedule?exp_flags=employee_current_date%3D2022-02-15)

## Доход исполнителя

`bonus_program_terms_accepted`  - флаг в `personal_info`  у организации, чтобы видеть отчеты в ерп
`has_got_salary`  - флаг у испа, чтобы отображать доход и отчеты в аппе

Изменение `bonus_program_terms_accepted`  (добавить запросу на UPDATE)  [YQL RUN](https://yql.yandex-team.ru/Operations/YmJcO7q3k_zTDUmheP2Uvf6S6RwgL9Ny97paQGSKr-U=)

## Изменение способа оплаты в заказе

Смс с кодом хранится в данных заказа в `internal_metadata` 
[YQL RUN Найти заказ по ID](https://yql.yandex-team.ru/Operations/Yk6OlwVK8Jv0ahbiIB3FjnUJZZ4t-Og5goZcuznTZV4=)

```json
{
    "change_payment_type_confirmation": {
        "send_ts": 1649315126,
        "code": "1385",
        "attempt_number": 26
    }
}
```

## Подтверждение расписания

Выставить статус или обновить статус расписания на любой день [YQL RUN](https://yql.yandex-team.ru/Operations/YouvtZfFtyqtuBws7W_CxO9JI4NrmP3uXEnrzYhFgfI=)

- Без флага мержа интервалов `yndx-ydo-employee-8`
- С флагом для мержа интервалов `yndx-ydo-employee-2`

Надо воссоздать ситуацию в тестинге в прошлое по испу, что он подтвердил расписание и выставил себе выходной.
Аккаунт - `yndx-ydo-employee-13` / `te$tY%do`

## Обязательные действия перед началом работы

Приходят в массиве `mandatory_actions`

#### Проверка паспорта

Разрабатывалась и управляется Службой контроля качества.

#### Обязательные курсы

```
{
"type": "education",
"kiosk_url": "https://instructor.vmb.co/?programUuid=program-uuid&externalId=some-employee-id&organization_id=some-organization-id",
"title": "Правила уборки",
"image": "https://...", <- обложка то, если не пришла - используем обложку от проверки паспорта
"status": "not_completed",
"create_dt": "2021-12-15T10:00:00.000Z
}
```

[Задача YDO-31852](https://st.yandex-team.ru/YDO-31852). В ней расписано более подробно о курсах на момент разработки.

[Админка с программами](https://education.adm.training.yandex/programs). В админке заведены как боевые курсы, так и тестовые (dev).
Доступ можно попросить у [Александра Балашова](https://staff.yandex-team.ru/alex-lore).
Чат с Киоском есть в телеграм, добавить может [Данил Гаманов](https://staff.yandex-team.ru/danil-gi), [Марат Хасанов](https://staff.yandex-team.ru/khasmar) или [Антон Захаров](https://staff.yandex-team.ru/antonzakharov).

* Курсы добавлены через iframe (сами курсы - разработка Киоска). 
* После прохождения в админке нужно настроить редирект на страницу `mandatory-actions/ready`, которая отправляет сообщение в приложении о том, что курс завершен и нужно вернуться на список курсов.
* Сейчас курс имеет только два статуса `completed` и `not_completed`. Если курс начат , но не закончен - отображется как не пройденный. 
* Если сессия еще жива - курс начинается с того места, где пользователь прервался. Время сессии указывается в админке курса. Дефолтное значение - 2 часа.
* Серая подложка - атрибут самого курса.

[YQL RUN Сбросить статус назначенного курса](https://yql.yandex-team.ru/Operations/Yd_umlZ1OyS7_F92exfVi_QKsrWciM47HUSX5emqnGE=)

## Сканирование штрихкодов

[YDO-31954 Научиться считывать штрих код](https://st.yandex-team.ru/YDO-31954)
[Форматы штрихкодов](https://st.yandex-team.ru/YDO-31954#61e6de27a0dfdb106aac88a4)
Онлайн формирование штрихкодов [Code128](https://barcode.tec-it.com/ru/Code128?data=0000000000383)
Сгенерировать последовательность кодов можно [тут](https://tamali.net/barcode/linear/Code-128/) или [тут](https://service-online.su/text/generator-shtrih-koda/)


{% cut "Проверка js-api" %}
```javascript
const onMessage = (event) => {
    try {
        if (typeof event.data !== 'string') {
            return;
        }

        const data = JSON.parse(event.data);

        console.log(event.data);
    } catch (ignored) {}
};

window.addEventListener('message', onMessage);

window.UslugiMobile.scanBarCode(JSON.stringify({format: 'FORMAT_CODE_128', title: 'Сканирование штрихкода', subtitle: 'Найдите штрихкод на этикетке и наведите на\xa0него\xa0камеру', manualButtonText: 'Ввести код вручную'}));
```
{% endcut %}

{% cut "Через обёртку" %}
```javascript
const hasRequestId = (data) => {
    return typeof data === 'object' && data !== null && 'requestId' in data;
};

const isMobappApiResult = (data, requestId) => {
    return hasRequestId(data) && data.requestId === requestId;
};


class MobappRawApi {
    constructor(globalProperty) {
        this.globalProperty = globalProperty;
    }

    invokeFn(methodName, params) {
        const object = window[this.globalProperty]

        const method = object[methodName];

        return params === null ? method.call(object) : method.call(object, JSON.stringify(params));
    }

    invokeAsyncFn(methodName, params) {
        const result = this.invokeFn(methodName, params);

        return result;
    }

    invokeAsyncApi(methodName, params) {
        return new Promise((resolve, reject) => {
            const requestId = this.invokeAsyncFn(methodName, params);

            const onMessage = (event) => {
                try {
                    if (typeof event.data !== 'string') {
                        return;
                    }

                    const data = JSON.parse(event.data);

                    if (isMobappApiResult(data, requestId)) {
                        window.removeEventListener('message', onMessage);

                        if (data.ok) {
                            resolve(data.payload);
                        } else {
                            reject(data.payload);
                        }
                    }
                } catch (ignored) {}
            };

            window.addEventListener('message', onMessage);
        });
    }

    invokeSyncApi(methodName, params) {
        return this.invokeFn(methodName, params);
    }
}

const mobileAppRawApi = new MobappRawApi('UslugiMobile');

mobileAppRawApi
    .invokeAsyncApi('scanBarCode', {format: 'FORMAT_CODE_128', title: 'Сканирование штрихкода', subtitle: 'Найдите штрихкод на этикетке и наведите на\xa0него\xa0камеру', manualButtonText: 'Ввести код вручную'})
    .then(a => console.log(a))
    .catch(e => console.log(e));
```
{% endcut %}

## Химчистка - Наша доставка

#### Создание заказа

1. Создаём заказ химчистки на Москву на [тестинге](https://shared-dev.hamster.yandex.ru/uslugi/). На заказ назначается исполнитель, курьеров у заказчика не видно.
2. Находим админке по id созданный заказ [пример](https://renderer-ydo-admin-master.admin-dev.ydo.yandex-team.ru/uslugi-admin/zakazator?service_order_form_id=ded094bf-2f44-4724-a687-00765ca24665&zakazator_tab=zakazator_order)
3. В амдминке в блоке "Связанные заказы" в правой части видим наш заказ "Забор вещей" [пример](https://renderer-ydo-admin-master.admin-dev.ydo.yandex-team.ru/uslugi-admin/zakazator?service_order_form_id=0a73a1e7-40c1-4c72-98f4-ee773707953e&zakazator_tab=zakazator_order)
4.  Ищем заказ в ерп и назначаем нужного курьера, если необходимо [пример](https://shared-dev.hamster.yandex.ru/uslugi/erp/orders?orderStatus=waiting_for_description&quickFilter=waiting_for_description). Удобно зайти под `yndx-ydo-partner-admin-1@yandex.ru / te$tY%do`.  Заказ до ЕРП доходит не сразу, как и любые изменения, нужно время на синк.

Переходы по статусам [delivery_bags.gml](https://a.yandex-team.ru/arc_vcs/ydo/erp/server/bin/backend/order_state_graphs/delivery_bags.gml)

#### Запросы для кодов

- Добавить barcode в список доступных [YQL RUN](https://yql.yandex-team.ru/Operations/YfPEU9JwbHujiVQaDbmTI2s47l1n30jc2adW1-6R6Ok=)
- Отвязать баркод от метазаказа [YQL RUN](https://yql.yandex-team.ru/Operations/Yfy0_bq3k_vkr-s2xCfeSE1qzGr06SqKmj0ZxWoWbi8=)
- Доступные для привязки баркоды [YQL RUN](https://yql.yandex-team.ru/Operations/YfPFVpfFt8LH3U-2kia0WAv0s8gFhlFuAJFDQ82nAVg=)
- Все баркоды [YQL RUN](https://yql.yandex-team.ru/Operations/YfPEua5ODzHFlHlMXccHZHNSkb_4kg0YJGWvnxIBdb8=)

## ПЦР со сканированием штрихкодов

Акк испа:
* Тестинг
1. `yndx-ydo-employee-2 / te$tY%do`
Партнер OOO Virgin Cleaning (анализатор анализоы)
2. `yndx-ydo-employee-med-1 / te$tY%do`
ООО Мед Анализы

* Прод
- Компания ООО Тестовая Все Категории 2я `yndx-ydo-partner-4@yandex.ru / te$tY%do `
- Исп `klimova.ksenija9714 / E.VYnr45`

#### Создание заказа на проде
- Залипаем на проде под `575836` (диспатч, чтобы заказы попадали на нашего исполнителя) 
- Заходим в ЕРП под `yndx-ydo-partner-4@yandex.ru / te$tY%do` и подтверждаем заказ

#### Смс, отправляемая клиенту 
[YQL RUN ищем pcr_form_order_reminder_general](https://yql.yandex-team.ru/Operations/YihNhq5OD58xD1KW7UzJhSSiD-cqHtav0Xk0BfBVpg4=)

## Яндекс Чаты

- Проставить чаты в базу [YQL RUN](https://yql.yandex-team.ru/Operations/YnzxqVZ1O6FkEfV0ZXGEQTEAvXmSGML9PmBR323lfV4=)
- Стереть контакты [YQL RUN](https://yql.yandex-team.ru/Operations/YnznZ65OD4EIKkCtJdlO0oJtRHXhPTi8zT6VPjddQ_A=)

## Флоу воды (nearest_order)

Приходит в массиве `app_themes`
