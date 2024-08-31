# Тестирование

##  Запуск тестов
Описан на странице [{#T}](quick-start-guide.md)

##  Мокирование апи
В Услугах и монорепе используется мокирование ответов апи через дампы (подробнее можно почитать тут).
С ними есть несколько проблем:
* При перезаписи тестов, нужно подготавливать аккаунт и перезаписывать тест целиком
* При добавлении параметра в запрос или при добавлении запроса в компонент ломаются все тесты, в которых подгружается этот компонент
* При нехватке какого-либо дампа падает бесполезная ошибка "не смогли прочитать файл ...(путь с хешом)"
* Данные в erp периодически рефетчатся и дампы будут причиной нестабильности
* Неудобно смотреть что лежит в дампе и изменять это
* Нельзя шарить замоканные ответы со сторибук тестами

Из-за этого мы используем мокирование апи на уровне девтулзов через [browser.mock(...)](https://webdriver.io/docs/api/browser/mock/). Это накладывает на нас ограничение в запуске тестов - они должны быть изолированы друг от друга, поэтому в конфиге гермионы выставляем `testsPerSession: 1` для каждого теста запускается отдельный браузер.

В коде тестов можно мокать через задание ответа или через функцию, если ответ метода должен меняться в ходе теста:
```javascript
let orderResponse = initialOrderState; // задаем переменную, значение которой будет возвращаться в ответе на order/ запрос
await browser.yaMockApi('order/', () => orderResponse); // на запросы order/ отдаем orderResponse
await browser.yaMockApi('calendar', calendarResponse); // на запрос calendar отдаем calendarResponse
await browser.yaMockApi('get_employees', employeesResponse, false); // всегда (аргумент false) на запрос get_employees отдаем employeesResponse
await browser.yaMockApi( // при запросе cancel_order, у которого в теле order_id равен id из мока меняем значение, которое вернет мок order/
    {
        method: 'cancel_order',
        filters: {
            postData: data => JSON.parse(data).data.params.order_id === orderResponse.order.id,
        },
    },
    () => {
        orderResponse.order.status = 'canceled';
        orderResponse.allowed_actions = [];
        return orderResponse;
    },
);
```

При открытии нового окна использованные моки необходимо перевызвать.

## Ссылки по теме
* [Тикет](https://st.yandex-team.ru/FRONTCOM-85) на лучшие практики автотестов в Яндексе
* [Рецепты](https://doc.yandex-team.ru/si-infra/hermione/hermione_recepty/) от фронтент инфры
* [Гитхаб гермионы](https://github.com/gemini-testing/hermione)
* [Документация](https://webdriver.io/) и [гитхаб](https://github.com/webdriverio/webdriverio/) вебдрайвера (запускается под капотом гермионы)
* [Тред](https://yndx-uslugi.slack.com/archives/C0L9ELJRX/p1633594626247900) про настройки для параллельного запуска тестов
* [ya commands Гермионы](https://arcanum.yandex-team.ru/arcadia/frontend/packages/hermione-ya-commands/src/commands?rev=r8998965)
