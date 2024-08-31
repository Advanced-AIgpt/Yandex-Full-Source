# Псевдообъект xscript.HttpRequest

В объекте частично реализована технология [XMLHttpRequest](http://ru.wikipedia.org/wiki/XMLHttpRequest) основываясь на [второй версии](http://www.w3.org/TR/XMLHttpRequest2/) ее спецификации.

**Список функций:**

- open;
- send;
- abort;
- setRequestHeader;
- getResponseHeader;
- getAllResponseHeaders;
- waitResponse.

#### `open`

Определяет метод, URL и требование асинхронного выполнения запроса.

**Входные параметры**:

- Метод. Он может быть любым: GET, POST, PUT, DELETE, HEAD...;
- URL;
- Async. Требование асинхронного выполнения запроса. Необязательный параметр, значение по умолчанию — `true`.

#### `send`

Отправляет запрос на сервер.

**Входные параметры:**

- Данные. Параметр необязательный. Значение по умолчанию — `null`.

**Комментарий:**

Если функция `open` была вызвана с параметром `async=true`, то запрос направляется в `thread-pool`, а `js`-блок продолжает работу дальше.

#### `abort`

Отменяет текущий запрос, удаляет все заголовки, ставит текст ответа сервера в `null`.

#### `setRequestHeader`

Добавляет HTTP-заголовок к уже имеющимся заголовкам запроса, за исключением Cookie, Cookie2, Referer, User-Agent. Если добавляемый заголовок уже существует, то оба значения комбинируются.

**Входные параметры:**

- Имя заголовка;
- Значение заголовка.

#### `getResponseHeader`

Возвращает значение указанного заголовка.
 Если флаг ошибки равен `true`, возвращает `null`.
 Если заголовок не найден, возвращает `null`.
 Если статус (значение свойства `status`) 0 или 1, вызывает ошибку `INVALID_STATE_ERR`.

**Входные параметры:**

- Имя заголовка.

#### `getAllResponseHeaders`

Возвращает полный список HTTP-заголовков в виде строки. Заголовки разделяются знаками переноса (CR+LF).
 Если флаг ошибки равен `true`, возвращает пустую строку.
 Если статус (значение свойства `status`) 0 или 1, вызывает ошибку `INVALID_STATE_ERR`.

#### `waitResponse`

Ожидает ответ при асинхронном запросе. Реализует следующую логику, если не было вызовов `open/send`, то вызывает `abort()`, в противном случае остнавливает выполнение `js`, ждет `readyState = 4`. Функция возвращает код ошибки от программы [cURL](http://ru.wikipedia.org/wiki/CURL). В случае успешного завершения возвращает 0.

**Свойства объекта:**

#|
|| Свойство | Описание ||
|| readyState | Текущее состояние объекта (0 — не инициализирован, 1 — открыт, 2 — отправка данных, 3 — получение данных и 4 — данные загружены). Свойство доступно только для чтения. ||
|| responseText | Текст ответа на запрос. Если состояние не 3 или 4, возвращает пустую строку. При заполнении этого свойства XScript пытается использовать кодировку из `Content-Type` для перекодирования в UTF-8. Если не получилось сконвертировать в UTF-8, то производится попытка сконвертировать содержимое из fallback-кодировки (cp1251). Если это не получилось, то свойство принимает значение <q>not utf-8 data</q>. Свойство доступно только для чтения. ||
|| status | HTTP-статус в виде числа (404 — <q>Not Found</q>, 200 — <q>OK</q> и т. д.). Свойство доступно только для чтения. ||
|| statusText | Статус в виде строки (<q>Not Found</q>, <q>OK</q> и т. д.). Если статус не распознан, браузер пользователя должен вызвать ошибку INVALID_STATE_ERR. Свойство доступно только для чтения. ||
|| timeout | Время ожидания выполнения запроса в миллисекундах. По истечении этого времени запрос прерывается. Значение по умолчанию 5000мс. Свойство доступно для чтения/записи. ||
|| errorText | Содержит текст ошибки от [cURL](http://ru.wikipedia.org/wiki/CURL) или текст внутренней ошибки. Свойство доступно только для чтения. ||
|# 

**Примеры:**

Пример 1.

```javascript
var handlers = {
    'offers': {'url': 'http://....', 'mandatory': true, 'timeout': 2500},
    'map-points': {'url': 'http://....', 'mandatory': true, 'timeout': 1500},
    'shops': {'url': 'http://....', 'mandatory': true, 'timeout': 1500},
    'shop-grades': {'url': 'http://....', 'mandatory': false, 'timeout': 1500}
}

for(var h in handlers){
    h.request = new xscript.HttpRequest();
    h.request.open("get", h.url);
    h.request.timeout = h.timeout || 0;
    h.request.send();
}

// тут еще много-много строк кода...

// контролируем ответы и сбрасываем долгие вызовы, если надо
for(var h in handlers){
    if(h.mandatory){
        h.request.waitResponse();
    } else if(h.request.readyState != 4){
        h.request.abort();
    }
}
```

Пример 2.

```
var params = xscript.state['@params'];
if (params.stamp) {
    var stampUrl = xscript.state.host + params.id + '/stamp.xml';
    var request = new xscript.HttpRequest();
    request.open('GET', stampUrl, false);
    request.send(null);
    if (request.status == 200) {
        var response = request.responseText;
        var responseXML = new XML(response.slice(response.indexOf('\n')));
        var lrs = new Namespace('http://maps.yandex.ru/layers/1.x');
        xscript.print(responseXML..lrs::version);
    }
}
```

### Узнайте больше {#learn-more}
* [JavaScript-блок](../concepts/block-js-ov.md)
* [js](../reference/js.md)