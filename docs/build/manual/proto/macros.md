# Protobuf : макросы

- [GRPC](#grpc)
- [SRCS](#srcs)
- [CPP_PROTO_PLUGIN](#cpp_proto_plugin)
- [CPP_PROTO_PLUGIN0](#cpp_proto_plugin0)
- [CPP_PROTO_PLUGIN2](#cpp_proto_plugin2)
- [EXCLUDE_TAGS](#exclude_tags)
- [GO_PROTO_PLUGIN](#go_proto_plugin)
- [INCLUDE_TAGS](#include_tags)
- [JAVA_PROTO_PLUGIN](#java_proto_plugin)
- [NO_OPTIMIZE_PY_PROTOS](#no_optimize_py_protos)
- [ONLY_TAGS](#only_tags)
- [OPTIMIZE_PY_PROTOS](#optimize_py_protos)
- [PROTO_ADDINCL](#proto_addincl)
- [PROTO_NAMESPACE](#proto_namespace)
- [PY_PROTO_PLUGIN](#py_proto_plugin)
- [PY_PROTO_PLUGIN2](#py_proto_plugin2)
- [USE_COMMON_GOOGLE_APIS](#use_google_common_apis)
- [USE_JAVA_LITE](#use_java_lite)
- [USE_LITE](#use_lite)


## GRPC
```GRPC()```

Вызов макроса `GRPC` в `PROTO_LIBRARY` добавляет использование `gRPC` плагина для команд `protoc`. Макрос `GRPC` реализован для всех языков, поддержанных в `Аркадии` (`C++`, `Go`, `Java`, `Python2`, `Python3`).

## SRCS
```SRCS(files)```

В макросе `SRCS` должны быть явно перечислены все `*.proto` необходимые для сборки [PROTO_LIBRARY](modules.md#proto_library).

{% note info %}

Maкрос `SRCS` в [PROTO_LIBRARY](modules.md#proto_library) также поддерживает файлы `.gztproto`, но только для `C++` и `Python` (для остальных языков `.gztproto` файлы игнорируются).

{% endnote %}

## CPP_PROTO_PLUGIN
```CPP_PROTO_PLUGIN(NAME, TOOL, SUF, DEPS[])```

Вызов макроса `CPP_PROTO_PLUGIN` позволяет добавить плагин для `C++` с именем `NAME` (`--protoc-gen-<Name> ... --<NAME>_out ...`), расположенный по пути `TOOL`, для запуска команды `protoc`, который сгенерирует дополнительные файлы с расширением `SUF` для каждого `.proto` файла из `SRCS`. Дополнительные зависимости, необходимые для сборки сгенерированного кода, перечисляются в параметре `DEPS`. 

## CPP_PROTO_PLUGIN0
```CPP_PROTO_PLUGIN0(NAME, TOOL, DEPS[])```

Вызов макроса `CPP_PROTO_PLUGIN0` позволяет добавить плагин для `C++` с именем `NAME` (`--protoc-gen-<NAME> ... --<NAME>_out ...`), расположенный по пути `TOOL`, для команды запуска `protoc`. Этот макрос используется в том случае, если код сгенерированный плагином будет записан в "умолчательные" выходные файлы команды `protoc` для `C++` (`.pb.h` и `.pb.cc`), Дополнительные зависимости, необходимые для сборки сгенерированного кода, перечисляются в параметре `DEPS`.

## CPP_PROTO_PLUGIN2
```CPP_PROTO_PLUGIN(NAME, TOOL, SUF1, SUF2, DEPS[])```

Вызов макроса `CPP_PROTO_PLUGIN2` позволяет добавить плагин для `C++` с именем `NAME` (`--protoc-gen-<Name> ... --<NAME>_out ...`), расположенный по пути `TOOL`, для запуска команды `protoc`, который сгенерирует дополнительные файлы с расширением `SUF1` и `SUF2` для каждого `.proto` файла из `SRCS`. Дополнительные зависимости, необходимые для сборки сгенерированного кода, перечисляются в параметре `DEPS`. 

## EXCLUDE_TAGS
```EXCLUDE_TAGS(Tags)```

Вызов макроса `EXCLUDE_TAGS` позволяет отключить инстанциацию подмодулей [PROTO_LIBRARY](modules.md#proto_library) для указанных вариантов (тэгов, перечисленных в аргументах макроса). По умолчанию инстанциируются следующие варианты [PROTO_LIBRARY](modules.md#proto_library): `CPP_PROTO`, `EXT_PROTO`, `GO_PROTO`, `JAVA_PROTO`, `PB_PY_PROTO`, `PY_PROTO`, `PY3_PROTO`.

## GO_PROTO_PLUGIN
```GO_PROTO_PLUGIN(NAME, EXT, TOOL, DEPS[])```

Вызов макроса `GO_PROTO_PLUGIN` позволяет добавить плагин для `GO` с именем `NAME` (`--protoc-gen-<Name> ... --<NAME>_out ...`), расположенный по пути `TOOL`, для запуска команды `protoc`, который сгенерирует дополнительные файлы с расширением `EXT` для каждого `.proto` файла из `SRCS`. Дополнительные зависимости, необходимые для сборки сгенерированного кода, перечисляются в параметре `DEPS`. 

## INCLUDE_TAGS
```INCLUDE_TAGS(Tags)```

Вызов макроса `INCLUDE_TAGS` позволяет добавить инстанциацию подмодулей [PROTO_LIBRARY](modules.md#proto_library) для указанных вариантов (тэгов, перечисленных в аргументах макроса).

## JAVA_PROTO_PLUGIN
```JAVA_PROTO_PLUGIN(NAME, TOOL, DEPS[])```

Вызов макроса `JAVA_PROTO_PLUGIN` позволяет добавить плагин для `Java` с именем `NAME` (`--protoc-gen-<Name> ... --<NAME>_out ...`), расположенный по пути `TOOL`, для запуска команды `protoc`. Дополнительные зависимости, необходимые для сборки сгенерированного кода, перечисляются в параметре `DEPS`. 

## NO_OPTIMIZE_PY_PROTOS
```NO_OPTIMIZE_PY_PROTOS()```

Вызов макроса отключает оптимизацию для вариантов `PY_PROTO` и `PY3_PROTO`. В этом случае будет использоваться нативный код на `Python` для сериализации сообщений. По умолчанию оптимизация включена, то есть используется `C++` код для сериализации из `Python`.

## ONLY_TAGS
```ONLY_TAGS(Tags)```

Вызов макроса `ONLY_TAGS` переопределяет набор инстанциаций подмодулей [PROTO_LIBRARY](modules.md#proto_library) для указанных вариантов (тэгов, перечисленных в аргументах макроса).

## OPTIMIZE_PY_PROTOS
```OPTIMIZE_PY_PROTOS()```

Вызов макроса включает оптимизацию кода для вариантов `PY_PROTO` и `PY3_PROTO`. В этом случае будет использоваться `C++` код для сериализации из `Python`.

## PROTO_ADDINCL
```PROTO_ADDINCL(GLOBAL? Dirs)```

Добавляет директорию поиска импортов для .proto-файлов, а также соответствующую директорию с генерированными файлами в пути поиска импортов для С++.
Параметр `GLOBAL` позволяет распространить это правило на все библиотеки, зависящие от данной. 

{% note alert %}

Мы не рекомендуем использовать этот макрос. В Аркадии приняты импорты от корня Аркадии, в особо сложных случаях стоит использовать [`PROTO_NAMESPACE`](#proto_namespace), который обеспечивает согласованность команд в модулях, где .proto описаны и где используются.

{% endnote %}

## PROTO_NAMESPACE
```
PROTO_NAMESPACE(GLOBAL? Dir)
```

Директория `Dir` будет корнем для генерируемого кода по .proto с точки зрения protoc. Это сделает все ссылки в генерированном коде от этой директории, а не от корня Аркадии.

{% note warning %}

* Директория `Dir` должна быть префиксом текущей модульной.
* Макрос `PROTO_NAMESPACE` должен идти до макросов [`GRPC`](#grpc) и `XXX_PROTO_PLUGIN`, поскольку все эти макросы формируют аутпуты с учётом namespace.

{% endnote %}

Параметр `GLOBAL` добавляет директорию к путям поиска импортов и инклудов, позволяя в зависящем коде ссылатьться на .proto (в импортах) и на .pb.h (в инклудах) от данной директории, а не от корня Аркадии.

Чтобы сделать возможность импортов в `Python` не от корня Аркадии надо использовать макрос [`PY_NAMESPACE`](../python/macros.md#py_namespace). При этом параметром [`PY_NAMESPACE`](../python/macros.md#py_namespace) должен
быть остаток пути от `PROTO_NAMESPACE` до директории с исходным кодом или `PY_NAMESPACE(.)` если вся директория проекта указана в `PROTO_NAMESPACE`. 

**Пример:**

Вот здесь https://a.yandex-team.ru/arc_vcs/infra/nanny/nanny_repo/ya.make?rev=r9082485#L20:
- `PROTO_NAMESPACE` - это `infra/nanny/nanny_repo`
- `PY_NAMESPACE` - это nanny_repo
- Исходный код лежит в `infra/nanny/nanny_repo/nanny_repo` (указано в `SRCDIR`).

## PY_PROTO_PLUGIN
```PY_PROTO_PLUGIN(NAME, TOOL, SUF, DEPS[])```

Вызов макроса `PY_PROTO_PLUGIN` позволяет добавить плагин для `Python` с именем `NAME` (`--protoc-gen-<Name> ... --<NAME>_out ...`), расположенный по пути `TOOL`, для запуска команды `protoc`, который сгенерирует дополнительные файлы с расширением `SUF` для каждого `.proto` файла из `SRCS`. Дополнительные зависимости, необходимые для сборки сгенерированного кода, перечисляются в параметре `DEPS`. 

## PY_PROTO_PLUGIN2
```PY_PROTO_PLUGIN(NAME, TOOL, SUF1, SUF2, DEPS[])```

Вызов макроса `PY_PROTO_PLUGIN` позволяет добавить плагин для `Python` с именем `NAME` (`--protoc-gen-<Name> ... --<NAME>_out ...`), расположенный по пути `TOOL`, для запуска команды `protoc`, который сгенерирует дополнительные файлы с расширением `SUF1` и `SUF2` для каждого `.proto` файла из `SRCS`. Дополнительные зависимости, необходимые для сборки сгенерированного кода, перечисляются в параметре `DEPS`. 

## USE_COMMON_GOOGLE_APIS
```USE_COMMON_GOOGLE_APIS([apis...])```

Добавляет в сборку модуля зависимость на `Common Google APIs`. Вызов макроса без аргументов добавляет зависимость на все `APIs`. Выбор конкретных `APIs` для сборки актуален только для `GO` (синтаксически поддержан для всех языков, то есть нет смысла делать вызов этого макроса под условием от типа собираемого подмодуля [PROTO_LIBRARY](modules.md#proto_library)) и связан с особенностью сборки для `GO`, где `APIs` представлены отдельными пакетами с сгенерированными `.pb.go` файлами. Для всех остальных языков, поддерживаемых в `Аркадии`, сборка производится из `.proto` файлов. 

## USE_JAVA_LITE
```USE_JAVA_LITE()```

Вызов `USE_JAVA_LITE` макроса переключает сборку на использование `java-lite` protobuf runtime-а для `Java`.

## USE_LITE
```USE_LITE()```

Вызов `USE_LITE` макроса переключает сборку на использование `java-lite` protobuf runtime-а для `Java`.


[https://wiki.yandex-team.ru/yatool/protolibrary/](https://wiki.yandex-team.ru/yatool/protolibrary/)
[http://clubs.at.yandex-team.ru/arcadia/16968](https://wiki.yandex-team.ru/yatool/protolibrary/)
