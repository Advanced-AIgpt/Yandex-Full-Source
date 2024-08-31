# Node.js

Интерфейс устанавливается пакетом `yandex-lang-detect-nodejs`.

Для использования в Node.js `lang-detect` предоставляет класс `LangDetector`.

**Методы класса:**

Метод | Описание
----- | -----
[LangDetector](#langdetector) | Конструктор класса.
[find](#find) | Возвращает язык отображения страницы.
[findWithoutDomain](#findWithoutDomain) | Определяет язык отображения страницы альтернативным алгоритмом, не учитывающим домен запроса.
[list](#list) | Возвращает список релевантных пользователю языков.
[findDomain](#finddomain) | Возвращает домен и регион пользователя.
[cookie2language](#cookie2language) | Преобразует числовой идентификатор языка в строковый.
[language2cookie](#language2cookie) | Преобразует строковый идентификатор языка в числовой.
[trySwap](#trySwap) | Обновление данных.


## LangDetector {#langdetector}

Конструктор класса.

```
function LangDetector(file)
```

**Входные параметры:**

Параметр | Описание
----- | -----
`file` | Местоположение файла `lang-detect-data.txt`. Обычно — `/usr/share/yandex/lang_detect_data.txt`.


## Метод find {#find}

Возвращает язык отображения страницы.

```
function find(param)
```

**Входные параметры:**

В качестве параметра принимает объект с полями:

Поле | Описание
----- | -----
geo | Регион пользователя
language | Язык из поля **Accept-Language** заголовка HTTP запроса.
cookie | Язык из куки **my**.
domain | Домен.
filter | Перечень доступных языков.
pass-language | [Язык](source-data.md) из **Паспорта**.
default | Язык сервиса по умолчанию.


**Возвращаемое значение:**

Объект с полями `id` и `name`.

**Пример:**

```javascript
var LangDetector = require('langdetect').LangDetector;

var l = new LangDetector('/usr/share/yandex/lang_detect_data.txt');

var res = l.find({'domain': 'http://mail.yandex.ru/neo2', 'filter': 'tt,ru,uk', 'geo': '24896,20529,20524,187,166,10001,10000'});

if (res) {
    console.log("id=" + res.id + " (" + res.name + ")");
}
```

Результат:

```
id=ru (Ru)
```

## Метод findWithoutDomain {#findWithoutDomain}

Возвращает язык отображения страницы.

```
function findWithoutDomain(param)
```

**Входные параметры:**

В качестве параметра принимает объект с полями:

Поле | Описание
----- | -----
geo | Регион пользователя
language | Язык из поля **Accept-Language** заголовка HTTP запроса.
cookie | Язык из куки **my**.
filter | Перечень доступных языков.
pass-language | [Язык](source-data.md) из **Паспорта**.
default | Язык сервиса по умолчанию.


**Возвращаемое значение:**

Объект с полями `id` и `name`.

**Пример:**

```javascript
var LangDetector = require('langdetect').LangDetector;

var l = new LangDetector('/usr/share/yandex/lang_detect_data.txt');

var res = l.findWithoutDomain({'filter': 'tt,ru,uk', 'geo': '24896,20529,20524,187,166,10001,10000'});

if (res) {
    console.log("id=" + res.id + " (" + res.name + ")");
}
```

Результат:

```
id=ru (Ru)
```

## Метод list {#list}

Возвращает список релевантных пользователю языков.

```
function list(param)
```

**Входные параметры:**

В качестве параметра принимает объект с полями:

Строка | Описание
----- | -----
geo | Регион пользователя
language | Язык из поля **Accept-Language** заголовка HTTP запроса.
domain | Домен.
cookie | Язык из куки **my**.
filter | Перечень доступных языков.
pass-language | [Язык](source-data.md) из **Паспорта**.
default | Язык сервиса по умолчанию.


**Возвращаемое значение:**

Массив объектов с полями `id` и `name`.

**Пример:**

```javascript
var LangDetector = require('langdetect').LangDetector;

var l = new LangDetector('/usr/share/yandex/lang_detect_data.txt');

var res = l.list({'domain': 'http://mail.yandex.ru/neo2', 'filter': 'tt,ru,uk', 'geo': '24896,20529,20524,187,166,10001,10000'});

if (res) {
    console.log(res);
}
```

Результат:

```javascript
[ { id: 'ru', name: 'Ru' }, { id: 'uk', name: 'Ua' } ]
```

## Метод findDomain {#finddomain}

Возвращает домен и регион пользователя.

```
function findDomain(region, domains, resource, cr_cookie)
```

**Входные параметры:**

Параметр | Описание
----- | -----
`region` | Регион пользователя.
`domains` | Перечень доступных для переадресации доменов.
`resource` | Запрошенный пользователем ресурс.
`cr_cookie` | `cr`-блок куки [куки yp](https://wiki.yandex-team.ru/cookies/y#yp).


**Возвращаемое значение:**

Объект со свойствами:

Свойство | Описание
----- | -----
`domain` | Домен.
`content-region` | Регион пользователя.
`changed` | True, если логикой библиотеки найден домен для переадресации.


**Пример:**

```javascript
var LangDetector = require('langdetect').LangDetector;

var l = new LangDetector('/usr/share/yandex/lang_detect_data.txt');

var res = l.findDomain('24896,20529,20524,187,166,10001,10000', 'ua,by,kz', 'http://mail.yandex.ru/neo2','ru');

if (res) {
    console.log(res);
}
```

Результат:

```javascript
{ host: 'mail.yandex.ua', changed: true, 'content-region': 24896 }
```

## Метод cookie2language {#cookie2language}

Преобразует числовой идентификатор языка в строковый.

```
function cookie2language(cookie-lang)
```

**Входные параметры:**

Параметр | Описание
----- | -----
`cookie-lang` | Числовой идентификатор языка.


**Возвращаемое значение:**

Строковый идентификатор языка.

**Пример:**

```javascript
var LangDetector = require('langdetect').LangDetector;

var l = new LangDetector('/usr/share/yandex/lang_detect_data.txt');

var res = l.cookie2language( 1 );
console.log("lang-id = " + res);

var res = l.cookie2language( 2 );
console.log("lang-id = " + res);
```

Результат:

```
lang-id = ru
lang-id = uk
```

## Метод language2cookie {#language2cookie}

Преобразует строковый идентификатор языка в числовой.

```
function language2cookie(lang)
```

**Входные параметры:**

Параметр | Описание
----- | -----
`lang` | Строковый идентификатор языка.


**Возвращаемое значение:**

Числовой идентификатор языка.

**Пример:**

```javascript
var LangDetector = require('langdetect').LangDetector;

var l = new LangDetector('/usr/share/yandex/lang_detect_data.txt');

var res = l.language2cookie( 'ru' );
console.log("cookie lang-id = " + res);

res = l.language2cookie( 'uk' );
console.log("cookie lang-id = " + res);
```

Результат:

```
cookie lang-id = 1
cookie lang-id = 2
```

## Метод trySwap {#trySwap}

Метод определяет изменился ли файл `lang-detect-data.txt` с момента создания объекта класса `LangDetector`.

```
function trySwap()
```

**Возвращаемое значение:**

Если файл изменился — `true`.

**Пример:**

```javascript
var LangDetector = require('langdetect').LangDetector;

var l = new LangDetector('/usr/share/yandex/lang_detect_data.txt');

var res = l.trySwap();

if (res) {
    console.log('Файл с данными изменился');
} else{
    console.log('Файл с данными не изменился');
}
```

Результат:

```
Файл с данными не изменился
```

