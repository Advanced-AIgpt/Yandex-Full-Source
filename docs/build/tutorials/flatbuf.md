# Как создать проект

## Прежде чем начать

Далее предполагается, что
- Локально имеется рабочая копия Аркадии полученная по [инструкции](https://docs.yandex-team.ru/devtools/intro/quick-start-guide).
- Разработчик знает в каком он проекте с точки зрения Аркадии (в какой папке верхнего уровня будет его код).
- Разработчик уже включён в одну из групп Арканум.
- Разработчик знаком с форматом сериализации данных [Flatbuffers](https://google.github.io/flatbuffers)

## Простейшая FBS_LIBRARY

Начнём с очень простого примера - с описания "страницы книги"

Пример [devtools/examples/tutorials/flatbuf/example1/library/](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/flatbuf/example1/library/).

Структура проекта:
```
devtools/examples/tutorials/flatbuf/example1/library/
├── schema.fbs
└── ya.make
```

Схема данных `schema.fbs`:

{% code '/devtools/examples/tutorials/flatbuf/example1/library/schema.fbs' %}

Код описания сборки (`ya.make`):

{% code '/devtools/examples/tutorials/flatbuf/example1/library/ya.make' %}

Каждый ya.make в Аркадии должен содержать владельца кода - того, к кому можно обратиться для исправления ошибок и одобрения изменений. Для этого служит макрос [OWNER](../manual/common/macros.md#owner). Желательно указывать сразу группу, чтобы владелец не терялся на время отпуска или при переходе в соседние подразделения.
В макросе [SRCS](../manual/flatbuf/macros.md#srcs) перечисляются все `.fbs` файлы необходимые для сборки [FBS_LIBRARY](../manual/flatbuf/modules.md#fbs_library).

`FBS_LIBRARY` описали, теперь можно приступить к сборке:
```
$ cd <arcadia-root>
$ ya make <project-dir>
```

Например:
```
$ ya make devtools/examples/tutorials/flatbuf/example1/library/
Ok
$ ls -1 devtools/examples/tutorials/flatbuf/example1/library/
flatbuf-example1-library.jar
flatbuf-example1-library-sources.jar
libpy3flatbuf-example1-library.global.a
libpyflatbuf-example1-library.global.a
library.a
schema.fbs
ya.make
$
```
или
```
$ cd devtools/examples/tutorials/protobuf/example1/page/
$ ./ya make 
Ok
$ ls -1
flatbuf-example1-library.jar
flatbuf-example1-library-sources.jar
libpy3flatbuf-example1-library.global.a
libpyflatbuf-example1-library.global.a
library.a
schema.fbs
ya.make
$
```

После окончания сборки в директории проекта появились файлы (а точнее символьные ссылки) `flatbuf-example1-library.jar`, `flatbuf-example1-library-sources.jar`, `libpy3flatbuf-example1-library.global.a`, `libpyflatbuf-example1-library.global.a`, `library.a`. Это артефакты cборки подмодулей `FBS_LIBRARY` - `JAVA_FBS`, `PY2_FBS`, `PY3_FBS` и `GO_PROTO` соответственно.

{% note info %}

Если проект `FBS_LIBRARY` указан непосредственно в целях построения или достижим транзитивно по `RECURSE`-ным связям, то в этом случае строятся все доступные варианты подмодулей. Если по каким-то причинам не все варианты подмодулей могут быть построены, то их можно исключить из сборки с помощью макросов [EXCLUDE_TAGS](../manual/flatbuf/macros.md#exclude_tags), [ONLY_TAGS](../manual/flatbuf/macros.md#only_tags). 

{% endnote %}

{% note info %}

Символьные ссылки в директории проекта появится только для `Linux` и `Darwin`. При построении на `Windows` ссылки на артефакты сборки не создаются внутри рабочей копии. Чтобы получить артефакты сборки воспользуйтесь дополнительными флагами `ya make`: `-o <output-dir>` или `-I <output-dir>`. ***Важно! не указывайте в качестве `<output-dir>` директорию внутри рабочей копии Аркадии.***

{% endnote %}


{% note info %}

В дополнение к артефактам сборки можно также получить сгенерированный исходный код `Flatbuffers` для соответствующих языков программирования указав дополнительный флаг сборки `--add-flatbuf-result`.

{% endnote %}

## FBS_LIBRARY c зависимостью
С ростом и развитием проекта возникает потребность разбить функциональность на части (библиотеки) или переиспользовать код из других проектов. Это справедливо и для модуля типа `FBS_LIBRARY`. Может возникнуть потребность разделить `FBS_LIBRARY` на части или использовать схемы из другой `FBS_LIBRARY`.

{% note warning %}

`FBS_LIBRARY` могут зависеть только от модулей типа `FBS_LIBRARY`.

{% endnote %}

Давайте рассмотрим на примере как организовать сборку в том случае, когда `FBS_LIBRARY` зависит от другой `FBS_LIBRRAY`. Немного усложним наш предыдущий пример - разделим `FBS_LIBRARY` из предыдущего примера на 2 части - описание страницы книги и, собственно, описание самой книги:

Пример [devtools/examples/tutorials/flatbuf/example2/](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/flatbuf/example2/).

Структура проекта:
```
devtools/examples/tutorials/flatbuf/example2
├── library
│   ├── book.fbs
│   ├── genre.fbs
│   └── ya.make
├── page
│   ├── page.fbs
│   └── ya.make
└── ya.make
```
Мы разделили схему [schema.fbs](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/flatbuf/example2/library/schema.fbs) (из предыдущего примера [example1](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/flatbuf/example1)) на 3 части [page.fbs](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/flatbuf/example2/page/page.fbs), [genre.fbs](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/flatbuf/example2/library/genre.fbs) и [book.fbs]((https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/flatbuf/example2/library/book.fbs)). А сборку разделили на два проекта (мультимодули `FBS_LIBRARY`): `page` и `library`.

Код описания сборки проекта `page` (`ya.make`):

{% code '/devtools/examples/tutorials/flatbuf/example2/page/ya.make' %}

Код описания сборки проекта `library` (`ya.make`):

{% code '/devtools/examples/tutorials/flatbuf/example2/library/ya.make' %}

Как видно, файл описания сборки `FBS_LIBRARY` для проекта `page` не отличается сильно от файла описания сборки `FBS_LIBRARY` для проекта `library`. Единственное и принципиальное отличие - это `PEERDIR(devtools/examples/tutorials/flatbufbuf/example2/page)` на зависимую `FBS_LIBRARY`.

## Использование FBS_LIBRARY из разных языков
[FBS_LIBRARY](../manual/flatbuf/modules.md#fbs_library) - это мультимодуль, с помощью которого можно собирать код сериализации данных для различных языков программирования, официально поддерживаемых в Аркадии (а именно: `C++`, `Go`, `Java`, `Python2`/`Python3`). Для того чтобы в сборке проекта можно было использовать код для `Flatbuffers` из определённой `FBS_LIBRARY`, необходимо в файле описании сборки проекта написать вызов макроса `PEERDIR` с путём до `FBS_LIBRARY` относительно корня `Аркадии`. Выбор подмодуля мультимодуля `FBS_LIBRARY`, необходимый для сборки конкретного проекта, определяется типом модуля самого проекта, использующего `FBS_LIBRARY` - иначе говоря, код сериализации будет соответствовать языку, на котором написан проект. То есть для проектов на `C++` (модули типа [LIBRARY](../manual/cpp/modules.md#library), [PROGRAM](../manual/cpp/modules.md#program), [DLL](../manual/cpp/modules.md#dll) etc) сгенерируется код сериализации на `C++`, для проектов на `GO` (модули типа [GO_LIBRARY](../manual/go/modules.md#go_library), [GO_PROGRAM](../manual/go/modules.md#go_program), [GO_DLL](../manual/go/modules.md#go_dll) etc) сгенерируется код сериализации на `Go` и так далее.

Примеры использования `FBS_LIBRARY` для языков, официально поддержанных в `Аркадии`:
- [C++](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/flatbuf/example3/cpp_program)
- [Go](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/flatbuf/example3/go_program)
- [Java](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/flatbuf/example3/java_program)
- [Python2](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/flatbuf/example3/py2_program)
- [Python3](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/flatbuf/example3/py3_program)

