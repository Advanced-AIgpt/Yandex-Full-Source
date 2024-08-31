# Protobuf : модули

Модули описывающие сборку `Protobuf`
- [PROTO_LIBRARY](#proto_library)

## PROTO_LIBRARY
```
PROTO_LIBRARY()
```
Это мультимодуль, описывающий сборку proto-библиотеки. В `PROTO_LIBRARY` определён следующий набор доступных вариантов: `CPP_PROTO` - `LIBRARY` для `C++`, `GO_PROTO` - `GO_LIBRARY` для `Go`, `JAVA_PROTO` - `EXTERNAL_JAVA_LIBRARY` для `Java`, `PY_PROTO` - `PY2_LIBRARY` для `Python2` и `PY3_PROTO` - `PY3_LIBRARY` для `Python3`. Набор доступных вариантов в каждой конкретной `PROTO_LIBRARY` можно изменить с помощью макросов [EXCLUDE_TAGS](macros.md#exclude_tags), [INCLUDE_TAGS](macros.md#include_tags) и [ONLY_TAGS](macros.md#only_tags). `PROTO_LIBRARY` используется в сборке как обычный модуль.

Пример (отключение сборки `GO_PROTO` и `JAVA_PROTO` вариантов):
```
OWNER(g:ymake)

PROTO_LIBRARY()

EXCLUDE_TAGS(GO_PROTO JAVA_PROTO)

SRCS(api.proto)

END()
```

На `PROTO_LIBRARY` можно ставить `PEERDIR` из модулей других типов, но в сборку будет попадать конкретный вариант `PROTO_LIBRARY` в соответствии с типом зависимого модуля (выбор варианта зависит от значений модульных переменных `MODULE_TAG` и `PEERDIR_TAGS`, определённых для варианта `PROTO_LIBRARY` и зависимого модуля соответственно, например: для модулей типа `LIBRARY`, `PROGRAM`, `DLL`, `UNITTEST` будет выбран `CPP_PROTO`; для модулей типа `GO_LIBRARY`, `GO_PROGRAM`, `GO_DLL`, `GO_TEST`, `GO_TEST_FOR` будет выбран вариант `GO_PROTO`; для модулей типа `JAVA_LIBRARY`, `JAVA_PROGRAM`, `EXTERNAL_JAVA_LIBRARY`, `JTEST`, `JUNIT5` будет выбран вариант `JAVA_PROTO`; для модулей типа `PY2_LIBRARY`, `PY2_PROGRAM` `PY2TEST` будет выбран `PY_PROTO`; `PY3_LIBRARY`, `PY3_PROGRAM` `PY3TEST` будет выбран `PY3_PROTO`.

Набор макросов, который можно вызывать в `PROTO_LIBRARY` довольно ограничен. Как правило "языкоспецифичные" макросы не могут быть использованы в `PROTO_LIBRARY`. Связано это с тем, что макросы должны поддерживаться для всех вариантов (Например, [PEERDIR](../common/macros.md#peerdir) или [GRPC](macros.md#grpc)).
Некоторые макросы, разрешённые для использования в `PROTO_LIBRARY`, поддержаны только в части вариантов `PROTO_LIBRARY`, а в оставшихся вариантах просто игнорируются (Например: вызовы макросов [GENERATE_ENUM_SERIALIZATION](../cpp/macros.md#generate_enum_serialization) и [GENERATE_ENUM_SERIALIZATION_WITH_HEADER](generate_enum_serialization_with_header) используются только в`CPP_PROTO` варианте, а вызов макроса [USE_COMMON_GOOGLE_APIS](macros.md#use_common_google_apis) с параметрами имеет значение только для варианта `GO_PROTO` в силу особенностей сборки для `Go`).

{% note info %}

Если без "языкоспецифичных" макросов не обойтись, есть возможность выписать условия в зависимости от варианта `PROTO_LIBRARY`. В каждом варианте `PROTO_LIBRARY` определена модульная переменная, имя которой совпадает с именем варианта ([CPP_PROTO](vars.md#cpp_proto), [GO_PROTO](vars.md#go_proto), [JAVA_PROTO](vars.md#java_proto), [PY_PROTO](vars.md#py_proto), [PY3_PROTO](vars.md#py3_proto)).

{% endnote %}

В макросе [SRCS](macros.md#srcs) перечисляются все исходные `.proto` файлы необходимые для сборки `PROTO_LIBRARY`. Если `PROTO_LIBRARY` зависит от других `PROTO_LIBRARY`, необходимо указать все такие зависимости в аргументах вызова макроса `PEERDIR`.

{% note info %}

В `PROTO_LIBRARY` могут также использоваться макросы кодогенерации (такие как `RUN_PROGRAM`, `PYTHON` etc) для генерации `.proto` файлов. В этом случае результаты генерации макросов (`.proto` файлы) должны быть перечислены в параметрах вызовов макросов кодогенерации с ключевым именем `OUT_NOAUTO` или `STDOUT_NOAUTO` и должны быть явно добавлены в вызовы макроса `SRCS`. Кроме того, сами вызовы макросов кодогенерации должны быть под условием `IF (GEN_PROTO)`.

Пример:
```
OWNER(g:ymake)

PROTO_LIBRARY()

SRCS(
    source.proto
    generated.proto
)

IF (GEN_PROTO)
    RUN_PROGRAM(path/to/generator STDOUT_NOAUTO generated.proto)
ENDIF()

END()
```

{% endnote %}



[https://wiki.yandex-team.ru/yatool/protolibrary/](https://wiki.yandex-team.ru/yatool/protolibrary/)
[http://clubs.at.yandex-team.ru/arcadia/16968](http://clubs.at.yandex-team.ru/arcadia/16968)
