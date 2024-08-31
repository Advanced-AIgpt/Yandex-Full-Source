# Flatbuffers : модули

Модули, описывающие сборку [Flatbuffers](https://google.github.io/flatbuffers/) в `Аркадии`:

- [FBS_LIBRARY](#fbs_library)

## FBS_LIBRARY
```
FBS_LIBRARY()
```
Это мультимодуль, описывающий сборку `Flatbuffers` библиотеки. В `FBS_LIBRARY` определён следующий набор доступных вариантов подмодулей: `CPP_FBS` - `LIBRARY` для `C++`, `GO_FBS` - `GO_LIBRARY` для `Go`, `JAVA_FBS` - `JAVA_LIBRARY` для `Java`, `PY2_FBS` - `PY2_LIBRARY` для `Python2` и `PY3_FBS` - `PY3_LIBRARY` для `Python3`. Набор доступных вариантов подмодулей в каждой конкретной `FBS_LIBRARY` можно изменить с помощью макросов [EXCLUDE_TAGS](macros.md#exclude_tags), [INCLUDE_TAGS](macros.md#include_tags) и [ONLY_TAGS](macros.md#only_tags). `FBS_LIBRARY` используется в описании сборки как обычный модуль:

Пример (отключение сборки `GO_FBS` и `JAVA_FBS` вариантов):
```
OWNER(g:ymake)

FBS_LIBRARY()

EXCLUDE_TAGS(GO_FBS JAVA_FBS)

SRCS(api.fbs)

END()
```
На `FBS_LIBRARY` можно ставить `PEERDIR` из модулей других типов, но в сборку будет попадать конкретный вариант `FBS_LIBRARY` в соответствии с типом зависимого модуля (выбор варианта зависит от значений модульных переменных `MODULE_TAG` и `PEERDIR_TAGS`, определённых для варианта `FBS_LIBRARY` и зависимого модуля соответственно:

Тип модуля | Вариант
:--- | :---
LIBRARY, PROGRAM, DLL, UNITTEST | CPP_FBS
GO_LIBRARY, GO_PROGRAM, GO_DLL, GO_TEST, GO_TEST_FOR | GO_FBS
JAVA_LIBRARY, JAVA_PROGRAM, EXTERNAL_JAVA_LIBRARY, JTEST, JUNIT5 | JAVA_FBS
PY2_LIBRARY, PY2_PROGRAM, PY2TEST | PY2_FBS
PY3_LIBRARY, PY3_PROGRAM, PY3TEST | PY3_FBS


{% note info %}

Если без "языкоспецифичных" макросов не обойтись, есть возможность выписать условия в зависимости от варианта `FBS_LIBRARY`. В каждом варианте `FBS_LIBRARY` определена модульная переменная, имя которой совпадает с именем варианта ([CPP_FBS](vars.md#cpp_fbs), [GO_FBS](vars.md#go_fbs), [JAVA_FBS](vars.md#java_fbs), [PY_FBS](vars.md#py_fbs), [PY3_FBS](vars.md#py3_fbs)).

{% endnote %}

В макросе [SRCS](macros.md#srcs) перечисляются все исходные `.fbs` файлы необходимые для сборки `FBS_LIBRARY`. Если `FBS_LIBRARY` зависит от других `FBS_LIBRARY`, необходимо указать все такие зависимости в аргументах вызова макроса `PEERDIR`.

