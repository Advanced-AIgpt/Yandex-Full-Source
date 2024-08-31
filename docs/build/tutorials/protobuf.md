# Как создать проект на protobuf

## Прежде чем начать

Далее предполагается, что
- Локально имеется рабочая копия Аркадии полученная по [инструкции](https://docs.yandex-team.ru/devtools/intro/quick-start-guide).
- Разработчик знает в каком он проекте с точки зрения Аркадии (в какой папке верхнего уровня будет его код).
- Разработчик уже включён в одну из групп Арканум.
- Разработчик знаком с форматом сериализации данных [Protocol Buffers (aka Protobuf)](https://developers.google.com/protocol-buffers)

## Простейшая PROTO_LIBRARY

Начнём с очень простого примера - с описания "страницы книги"

Пример [devtools/examples/tutorials/protobuf/example1/page/](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/protobuf/example1/page/).

Структура проекта:
```
devtools/examples/tutorials/protobuf/example1/page
├── page.proto
└── ya.make
```

Код структуры сериализуемых данных `page.proto`:

{% code '/devtools/examples/tutorials/protobuf/example1/page/page.proto' lang='proto' %}

Код описания сборки (`ya.make`):

{% code '/devtools/examples/tutorials/protobuf/example1/page/ya.make' %}

Каждый ya.make в Аркадии должен содержать владельца кода - того, к кому можно обратиться для исправления ошибок и одобрения изменений. Для этого служит макрос [OWNER](../manual/common/macros.md#owner). Желательно указывать сразу группу, чтобы владелец не терялся на время отпуска или при переходе в соседние подразделения.
В макросе [SRCS](../manual/proto/macros.md#srcs) перечисляются все `.proto` файлы необходимые для сборки [PROTO_LIBRARY](../manual/proto/modules.md#proto_library).

`PROTO_LIBRARY` описали, теперь можно приступить к сборке:
```
$ cd <arcadia-root>
$ ya make <project-dir>
```

Например:
```
$ ya make devtools/examples/tutorials/protobuf/example1/page/
Ok
$ ls -1 devtools/examples/tutorials/protobuf/example1/page/
libprotobuf-example1-page.a
libpy3protobuf-example1-page.a
libpy3protobuf-example1-page.global.a
libpyprotobuf-example1-page.a
libpyprotobuf-example1-page.global.a
page.a
page.proto
protobuf-example1-page.jar
protobuf-example1-page.raw.fake.pkg
protobuf-example1-page-sources.jar
ya.make
$
```
или
```
$ cd devtools/examples/tutorials/protobuf/example1/page/
$ ./ya make 
Ok
$ ls -1
libprotobuf-example1-page.a
libpy3protobuf-example1-page.a
libpy3protobuf-example1-page.global.a
libpyprotobuf-example1-page.a
libpyprotobuf-example1-page.global.a
page.a
page.proto
protobuf-example1-page.jar
protobuf-example1-page.raw.fake.pkg
protobuf-example1-page-sources.jar
ya.make
$
```

После окончания сборки в директории проекта появились файлы (а точнее символьные ссылки) `libprotobuf-example1-page.a`, `libpy3protobuf-example1-page.a`, `libpyprotobuf-example1-page.a`, `page.a`, `protobuf-example1-page.jar`, `protobuf-example1-page-sources.jar`. Это артефакты cборки подмодулей `PROTO_LIBRARY` - `PY_PROTO`, `PY3_PROTO`, `CPP_PROTO`, `GO_PROTO` и `JAVA_PROTO` соответственно.

{% note info %}

Если проект `PROTO_LIBRARY` указан непосредственно в целях построения или достижим транзитивно по `RECURSE`-ным связям, то в этом случае строятся все доступные варианты подмодулей. Если по каким-то причинам не все варианты подмодулей могут быть построены, то их можно исключить из сборки с помощью макросов [EXCLUDE_TAGS](../manual/proto/macros.md#exclude_tags), [ONLY_TAGS](../manual/proto/macros.md#only_tags). 

{% endnote %}

{% note info %}

Символьные ссылки в директории проекта появится только для `Linux` и `Darwin`. При построении на `Windows` ссылки на артефакты сборки не создаются внутри рабочей копии. Чтобы получить артефакты сборки воспользуйтесь дополнительными флагами `ya make`: `-o <output-dir>` или `-I <output-dir>`. ***Важно! не указывайте в качестве `<output-dir>` директорию внутри рабочей копии Аркадии.***

{% endnote %}


{% note info %}

В дополнение к артефактам сборки можно также получить сгенерированный исходный код сериализации `Protobuf` для соответствующих языков программирования указав дополнительный флаг сборки `--add-protobuf-result`.

{% endnote %}

## PROTO_LIBRARY c зависимостью
С ростом и развитием проекта возникает потребность разбить функциональность на части (библиотеки) или переиспользовать код из других проектов. Это справедливо и для модуля типа `PROTO_LIBRARY`. Может возникнуть потребность разделить `PROTO_LIBRARY` на части или использовать сообщения из другой `PROTO_LIBRARY`.

{% note warning %}

В общем случае, `PROTO_LIBRARY` могут зависеть только от модулей типа `PROTO_LIBRARY`.

{% endnote %}

Давайте рассмотрим на примере как организовать сборку в том случае, когда `PROTO_LIBRARY` зависит от другой `PROTO_LIBRARY`. Немного усложним наш предыдущий пример - добавим новую `PROTO_LIBRARY`, в которой будет описана "очень простая книга" (сообщенеие `Book`, зависящее от сообщения `Page`):

Пример [devtools/examples/tutorials/protobuf/example2/](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/protobuf/example2/).

Структура проекта:
```
devtools/examples/tutorials/protobuf/example2
├── book
│   ├── book.proto
│   └── ya.make
├── page
│   ├── page.proto
│   └── ya.make
└── ya.make
```
Код описания сборки проекта `page` не изменился. Рассмотрим более подробно изменения, связанные с новым проектом `book`.

Код структуры сериализуемых данных `book.proto`:

{% code '/devtools/examples/tutorials/protobuf/example2/book/book.proto' lang='proto' %}

Код описания сборки (`ya.make`):

{% code '/devtools/examples/tutorials/protobuf/example2/book/ya.make' %}

Как видно, файл описания сборки `PROTO_LIBRARY` для проекта `book` не отличается сильно от файла описания сборки `PROTO_LIBRARY` для проекта `page`. Единственное и принципиальное отличие - это `PEERDIR(devtools/examples/tutorials/protobuf/example2/page)` на зависимую `PROTO_LIBRARY`.

## Использование PROTO_LIBRARY из разных языков
[PROTO_LIBRARY](../manual/proto/modules.md#proto_library) - это мультимодуль, с помощью которого можно собирать код сериализации данных для различных языков программирования, официально поддерживаемых в Аркадии (а именно: `C++`, `Go`, `Java`, `Python2`/`Python3`). Для того чтобы в сборке проекта можно было использовать код для сериализации сообщений [Protocol buffers](https://developers.google.com/protocol-buffers) из определённой `PROTO_LIBRARY`, необходимо в файле описании сборки проекта написать вызов макроса `PEERDIR` с путём до `PROTO_LIBRARY` относительно корня `Аркадии`. Выбор подмодуля мультимодуля `PROTO_LIBRARY` необходимый для сборки конкретного проекта определяется типом модуля самого проекта, использующего `PROTO_LIBRARY` - иначе говоря, код сериализации будет соответствовать языку, на котором написан проект. То есть для проектов на `C++` (модули типа [LIBRARY](../manual/cpp/modules.md#library), [PROGRAM](../manual/cpp/modules.md#program), [DLL](../manual/cpp/modules.md#dll) etc) сгенерируется код сериализации на `C++`, для проектов на `GO` (модули типа [GO_LIBRARY](../manual/go/modules.md#go_library), [GO_PROGRAM](../manual/go/modules.md#go_program), [GO_DLL](../manual/go/modules.md#go_dll) etc) сгенерируется код сериализации на `Go` и так далее.

Примеры использования `PROTO_LIBRARY` для языков, официально поддержанных в `Аркадии`:
- [C++](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/protobuf/example3/program)
- [Go](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/protobuf/example3/go_program)
- [Java](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/protobuf/example3/java_program)
- [Python2](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/protobuf/example3/py_program)
- [Python3](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/protobuf/example3/py3_program)

## gRPC

Давайте рассмотрим как описать построение простого `gRPC` сервиса в `Аркадии` и сделаем это на основе [helloworld](https://github.com/grpc/grpc/tree/master/examples/cpp/helloworld) примера с [github.com/grpc/grpc](https://github.com/grpc/grpc).

Код примера можно посмотреть здесь [devtools/examples/tutorials/protobuf/example4/](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/protobuf/example4/).

Структура проекта:
```
devtools/examples/tutorials/protobuf/example4
├── greeter_client
│   ├── greeter_client.go
│   └── ya.make
├── greeter_server
│   ├── greeter_server.cc
│   └── ya.make
├── helloworld
│   ├── helloworld.proto
│   └── ya.make
└── ya.make
```

В нашем примере код сервера [greeter_server](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/protobuf/example4/greeter_server) написан на `C++`, а код клиента [greeter_client](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/protobuf/example4/greeter_client) - на `Go`.

[helloworld](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/protobuf/example4/helloworld) - это `PROTO_LIBRARY` с описанием сервиса:

{% code '/devtools/examples/tutorials/protobuf/example4/helloworld/helloworld.proto' lang='proto' %}

Код описания сборки (`ya.make`):

{% code '/devtools/examples/tutorials/protobuf/example4/helloworld/ya.make' %}

Код описания сборки `PROTO_LIBRARY` для сервиса мало чем отличается от кода описания сборки `PROTO_LIBRARY`, рассмотренных в предыдущих примерах. Единственное отличие  - это вызов макроса [GRPC](../manual/proto/macros.md#grpc). Вызов этого макроса в описании`PROTO_LIBRARY` включает дополнительный плагин для `protoc`, который генерирует клиентский (стаб) и серверный (интерфейс) код. `gRPC` плагин поддерживается для всех официальных языков в `Аркадии`.

Давайте посмотрим какие промежуточные файлы будут сгенерированы для сборки `PROTO_LIBRARY` с `gRPC`. Для этого в строке сборки добавим флаг `--add-protobuf-result`.
```
$ ya make devtools/examples/tutorials/protobuf/example4/helloworld --add-protobuf-result
Ok
$ ls -1 devtools/examples/tutorials/protobuf/example4/helloworld
helloworld.a
helloworld.grpc.pb.cc
helloworld.grpc.pb.h
helloworld_pb2_grpc.py
helloworld_pb2.py
helloworld_pb2.pyi
helloworld.pb.cc
helloworld.pb.go
helloworld.pb.h
helloworld.proto
libprotobuf-example4-helloworld.a
libpy3protobuf-example4-helloworld.a
libpy3protobuf-example4-helloworld.global.a
libpyprotobuf-example4-helloworld.a
libpyprotobuf-example4-helloworld.global.a
protobuf-example4-helloworld.jar
protobuf-example4-helloworld.raw.fake.pkg
protobuf-example4-helloworld-sources.jar
ya.make
$
```

Кроме ссылок на артефакты сборки всех подмодулей `PROTO_LIBRARY`, появились ссылки на промежуточные файлы сборки, генерируемые `protoc` и `gRPC` плагином. Плагины для разных языков могут генерировать `gRPC` код как в "основные" выходные файлы (например, `GO` - `*.pb.go`), так и в дополнительные выходные файлы (например, `C++` - `*.grpc.pb.h` и `*.grpc.pb.cc`, `Python2`/`Python3` - `*_pb2_grpc.py`).

Код сервера ([greeter_server](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/protobuf/example4/greeter_server/greeter_server.cc)) написан на `C++`.

Код описания сборки (`ya.make`) сервера:

{% code '/devtools/examples/tutorials/protobuf/example4/greeter_server/ya.make' %}

В параметрах вызова макроса `PEERDIR` указаны все необходимые зависимости для сборки кода сервера.

Код клиента ([greeter_client](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/protobuf/example4/greeter_client/greeter_client.go)) написан на `Go`.

Код описания сборки (`ya.make`) клиента:

{% code '/devtools/examples/tutorials/protobuf/example4/greeter_client/ya.make' %}

Заметим, что из-за особенностей сборки для `Go` в `Аркадии` указывать зависимости в параметрах вызова макроса `PEERDIR` не надо. Все необходимые зависимости будут автоматически добавлены для сборки проекта на `GO` (за исключением зависимостей используемых в сгенерированных файлах).
