# Тестовый запрос в curl

Перед запросом убедитесь, что получили нужный API-ключ.


## Создание тестового вызова API {#test-call}

После установки сделайте тестовый вызов [метода complete](https://yandex.ru/dev/predictor/doc/dg/reference/complete.html) API Предиктора: 
```bash
curl -X GET "https://predictor.yandex.net/api/v1/predict/complete?q=wh&lang=en&key=ваш-API-ключ"
```

В ответе вернется: 
```xml
<?xml version="1.0" encoding="utf-8"?>
<CompleteResponse endOfWord="false" pos="-2"><text><string>which</string></text></CompleteResponse>
```
Перейдите на страницу с методом и прочтите из документации, что делает этот метод и API Предиктора.

Вам пришел минимизированный ответ. Его можно развернуть с помощью расширений в VS Code.

{% cut "Как развернуть код в VS Code" %}

Приведите код в удобный для чтения вид:

- В [магазине бесплатных расширений](https://marketplace.visualstudio.com/) найдите расширение Beautify или другое, преобразующее код, и установите его.
- Воспользуйтесь онлайн-сервисом [JSON Beautify](https://jsonbeautify.com/) или другим подобным.

Чтобы в VS Code развернуть ответ, который вернулся в curl:

1. В curl скопируйте текст ответа.
2. В VS Code создайте новый документ и вставьте в него скопированный текст.
3. Нажмите клавишу **F1** и в строке ввода команды напишите `Beautify file`.
4. Выберите язык, на который надо преобразовать текст.

{% endcut %}

## Параметры строки запроса {#query}

С помощью строки запроса в конечную точку были переданы:
- текст, на который указывает курсор пользователя (`q`);
- язык текста (`lang`);
- API-ключ (`key`).

Добавленный к URL знак `?` указывает на начало строки запроса. Дальше каждый параметр объединяется с другими параметрами через символ амперсанда `&`. Порядок параметров в строке запроса не имеет значения. Порядок имеет значение только если параметры являются частью самого URL-адреса (параметры пути).

Код для curl также можно извлекать из Postman. Для этого нажмите в интерфейсе Postman на панели справа значок ![](../images/code.png) и выберите в списке cURL.

## Опции cURL {#rest-api-commands}

По умолчанию в запросах curl используется метод `GET`. Для других HTTP-запросов указывайте нужный метод.

В команде можно указать несколько опций. Разделять опции нужно пробелом. Опции обозначаются отдельными буквами или целыми словами и фразами: `-L`, `-location`, `-location-trusted`. 

Опции из одного символа можно группировать, например:

```
curl -#0 https://example.com
```

то же самое, что и

```
curl -# -0 https://example.com
```

Для многих опций нужно указывать аргументы. Например:

```
curl -A Mozilla/5.0 https://www.example.com
```

В этой команде аргумент опции `-A` — это `Mozilla/5.0`.

Все опции логического типа (boolean) добавляются через `--имя` и убираются через `--no-имя`:

- `--verbose` или `-v` — включить подробный вывод.
- `--no-verbose` или `-no-v` — выключить подробный вывод.

Опции можно записывать с одним или двумя дефисами:

- Значения опций с одним дефисом можно писать слитно или раздельно с опцией, например `-d POST` или `-dPOST`. Рекомендуется писать раздельно.
- Значения опций с двумя дефисами необходимо всегда писать раздельно, например `--data @filename`.

У curl есть множество различных опций, которые вы включаете в свой запрос. Полный список смотрите на [официальном сайте curl](https://curl.se/docs/manpage.html).

{% cut "Таблица опций" %}

Опция | Описание | Структура запроса | Пример
----- | ----- | ----- | -----
`@` | Загрузить контент из файла. | ``` curl @<имя файла> <url> ``` | ``` curl -d @filename.json https://www.example.com ```
`-#` | Отображать прогресс-бар во время загрузки. | ``` curl -# <url> ``` | ``` curl -# https://www.example.com ```
`-0` | Использовать протокол http 1.0. | ``` curl -0 <url> ``` | ``` curl -0 https://www.example.com ```
`-1` | Использовать протокол шифрования tlsv1. | ``` curl -1 <url> ``` | ``` curl -1 https://www.example.com ```
`-2` | Использовать sslv2. | ``` curl -2 <url> ``` | ``` curl -2 https://www.example.com ```
`-3` | Использовать sslv3. | ``` curl -3 <url> ``` | ``` curl -3 https://www.example.com ```
`-4` | Использовать ipv4. | ``` curl -4 <url> ``` | ``` curl -4 https://www.example.com ```
`-6` | Использовать ipv6. | ``` curl -6 <url> ``` | ``` curl -6 https://www.example.com ```
`-A` | Указать свой USER_AGENT. | ``` curl -A <user_agent> <url> ``` | ``` curl -A Mozilla/5.0 https://www.example.com ```
`-b` | Сохранить Cookie в файл. | ``` curl -b <путь к файлу><имя файла> <url> ``` | ``` curl -b ./my/path/filename.json https://www.example.com ```
`-c` | Отправить Cookie на сервер из файла. | ``` curl -c <путь к файлу><имя файла> <url> ``` | ``` curl -c ./my/path/filename.json https://www.example.com ```
`-C` | Продолжить загрузку файла с места разрыва или указанного смещения. | ``` curl -C - <url> ``` | ``` curl -C - https://www.example.com ```
`-d` | Отправить данные методом POST. | ``` curl -d <данные> <url> ``` | ``` curl -d newdata https://www.example.com ```
`-D` | Сохранить заголовки, возвращенные сервером, в файл. | ``` curl -D <путь к файлу><имя файла> <url> ``` | ``` curl -D ./my/path/filename.json https://www.example.com ```
`-e` | Задать поле Referer-uri, указывает с какого сайта пришел пользователь. | ``` curl -e <url источника> <url> ``` | ``` curl -e https://example1.com https://www.example.com ```
`-E` | Использовать внешний сертификат SSL. | ``` curl -E <url сертификата> <url> ``` | ``` curl -E https://www.example.com/ssl https://www.example.com ```
`-f` | Не выводить сообщения об ошибках. | ``` curl -f <url> ``` | ``` curl -f https://www.example.com ```
`-F` | Отправить данные в виде формы. | ``` curl -F <данные> <url> ``` | ``` curl -F newdata https://www.example.com ```
`-G` | Использовать метод GET для данных в опции -d. | ``` curl -G -d <данные> <url> ``` | ``` curl -G -d newdata https://www.example.com ```
`-H` | Передать заголовки на сервер. | ``` curl -H <заголовки> <url> ``` | ``` curl -H "X-MyHeader: 123" https://www.example.com ```
`-help` | Показать справку. | ``` curl -help ``` | ``` curl -help ```
`-I` | Получать только HTTP заголовок, а все содержимое страницы игнорировать. | ``` curl -I <url> ``` | ``` curl -I https://www.example.com ```
`-j` | Прочитать и отправить cookie из файла. | ``` curl -j <путь к файлу><имя файла> <url> ``` | ``` curl -j ./my/path/filename.json https://www.example.com ```
`-J` | Удалить заголовок из запроса. | ``` curl -J <url> ``` | ``` curl -J https://www.example.com ```
`-L` | Принимать и обрабатывать перенаправления. | ``` curl -L <url> ``` | ``` curl -L https://www.example.com ```
`-m` | Максимальное время ожидания ответа от сервера. | ``` curl -m <секунды> <url> ``` | ``` curl -m 10 https://www.example.com ```
`-o` | Выводить контент страницы в файл. | ``` curl -o <путь к файлу><имя файла> <url> ``` | ``` curl -o ./my/path/filename.json https://www.example.com ```
`-O` | Сохранять контент в файл с именем страницы или файла на сервере. | ``` curl -O <путь к файлу><имя файла> <url> ``` | ``` curl -O ./my/path/filename.json https://www.example.com ```
`-p` | Использовать прокси. | ``` curl -p <url прокси> <url> ``` | ``` curl -p https://www.example.com/proxy https://www.example.com ```
`--proto` | Использовать определенный протокол. | ``` curl --proto <протокол> <url> ``` | ``` curl --proto =https https://www.example.com ```
`-R` | Сохранять время последнего изменения удаленного файла. | ``` curl -R <url> ``` | ``` curl -R https://www.example.com ```
`-S` | Выводить минимум информации об ошибках. | ``` curl -s <url> ``` | ``` curl -s https://www.example.com ```
`-S` | Выводить сообщения об ошибках. | ``` curl -S <url> ``` | ``` curl -S https://www.example.com ```
`-T` | Загрузить файл на сервер. | ``` curl -T <путь к файлу><имя файла> <url> ``` | ``` curl -T ./my/path/filename.json https://www.example.com ```
`-v` | Максимально подробный вывод. | ``` curl -v <url> ``` | ``` curl -v https://www.example.com ```
`-V` | Вывести версию. | ``` curl -V ``` | ``` curl -V ```
`-X` | Использовать определенный метод. | ``` curl -X <метод> <url> ``` | ``` curl -X POST @filename.json https://www.example.com ```
`-y` | Минимальная скорость загрузки. | ``` curl -y <число> <url> ``` | ``` curl -y 10 https://www.example.com ```
`-Y` | Максимальная скорость загрузки. | ``` curl -Y <число> <url> ``` | ``` curl -Y 100 https://www.example.com ```
`-z` | Скачать файл, только если он был модифицирован позже указанного времени. | ``` curl -z <время> <url> ``` | ``` curl -z 21-Dec-20 https://www.example.com ```


{% endcut %}