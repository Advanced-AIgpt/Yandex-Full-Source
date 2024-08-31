# Python : модули


Модули, описывающие Python-сборку
- [PY2_LIBRARY](#py_library)
- [PY3_LIBRARY](#py3_library)
- [PY2_PROGRAM](#py_program)
- [PY3_PROGRAM](#py3_program)
- [PY23_LIBRARY](#py23_library)
- [PY23_NATIVE_LIBRARY](#py23_native_library)
- [PY2MODULE/PY3MODULE](#pymodule)
- [PY_ANY_MODULE](#py_any_module)
- [PY2TEST / PY3TEST](#pytestpy3test)
- [PY23_TEST](#py23_test)


## PY2_LIBRARY


`PY2_LIBRARY()`


Библиотека для программ/тестов на Python 2.
**Макрос устарел. Используйте вместо него макрос [`PY23_LIBRARY`](#py23_library) или [`PY3_LIBRARY`](#py3_library) (подробнее см. тут: https://clubs.at.yandex-team.ru/arcadia/23044).**

- Собирает исходный код из [`PY_SRCS`](macros.md#py_srcs) в формате для использования в [`PY2_PROGRAM`](#py_program).

- Добавляет зависимость от Аркадийной runtime-библиотеки Python 2 и настраивает пути поиcка `Python.h` на Python 2 из Аркадии.

- При `PEERDIR` выбирает Python 2 вариант из [`PY23_LIBRARY`](#py23_library) и `PROTO_LIBRARY`. Может зависеть по `PEERDIR` только от Python 2 модулей.

Можно в | Нельзя в
:--- | :---
`PY2_LIBRARY` | `PY3_LIBRARY`
`LIBRARY` | `LIBRARY` c `USE_PYTHON3()`
`PY23_LIBRARY` | -
- | `PY2MODULE`, `PY_ANY_MODULE`, `PY23_NATIVE_LIBRARY`
- | `PY2_PROGRAM`, `PY2TEST`, `PY3_PROGRAM`, `PY3TEST`, `PY23_TEST`

- Библиотеку разрешено использовать только из Python 2 модулей, совместимых с Аркадийной сборкой Python.

Можно из | Нельзя из
:--- | :---
`PY2_LIBRARY`, `PY2_PROGRAM`, `PY2TEST` | `PY3_LIBRARY`, `PY3_PROGRAM`, `PY3TEST`
`PROGRAM`, `LIBRARY` | `PROGRAM`, `LIBRARY` c `USE_PYTHON3()`
`PY23_LIBRARY`, `PY23_TEST` под `IF (PYTHON2)` | `PY23_LIBRARY`, `PY23_TEST`  в остальных случаях
- | `PY2MODULE`, `PY_ANY_MODULE`, `PY23_NATIVE_LIBRARY`


**Пример:**
```
OWNER(g:my_group)
PY2_LIBRARY()
    PEERDIR(
        my/lib/py23
        my/lib/cpp
        my/lib/proto
    )

    PY_SRCS(x.py)
END()
```

{% note info %}

В силу особенностей сборки при непосредственной сборке такой библиотеки командой `ya make path/to/lib` у неё может совсем не быть артефакта.
Результатом может быть объектный файл, который непосредственно влинкуется в программу.

{% endnote %}


## PY3_LIBRARY


`PY3_LIBRARY()`


Библиотека для программ/тестов на Python 3.

- Собирает исходный код из [`PY_SRCS`](macros.md#py_srcs) в формате для использования в [`PY3_PROGRAM`](#py3_program).

- Добавляет зависимость от Аркадийной runtime-библиотеки Python 3 и настраивает пути поиска `Python.h` на Python 3 из Аркадии.

- При `PEERDIR` выбирает Python 3 вариант из [`PY23_LIBRARY`](#py23_library) и `PROTO_LIBRARY`. Может зависеть по `PEERDIR` только от Python 2 модулей.

Можно в | Нельзя в
:--- | :---
`PY3_LIBRARY` | `PY2_LIBRARY`
`LIBRARY` c `USE_PYTHON3()` | `LIBRARY`
`PY23_LIBRARY` | -
- | `PY2MODULE`, `PY_ANY_MODULE`, `PY23_NATIVE_LIBRARY`
- | `PY2_PROGRAM`, `PY2TEST`, `PY3_PROGRAM`, `PY3TEST`, `PY23_TEST`


- Библиотеку разрешено использовать только из Python 3 модулей, совместимых с Аркадийной сборкой Python.

Можно из | Нельзя из
:--- | :---
`PY3_LIBRARY`, `PY3_PROGRAM`, `PY3TEST` | `PY2_LIBRARY`, `PY2_PROGRAM`, `PY3TEST`
`PROGRAM`, `LIBRARY`  c `USE_PYTHON3()` | `PROGRAM`, `LIBRARY` в остальных случаях
`PY23_LIBRARY`, `PY23_TEST` под `IF (PYTHON3)` | `PY23_LIBRARY`, `PY@#_TEST`  в остальных случаях
- | `PY2MODULE`, `PY_ANY_MODULE`, `PY23_NATIVE_LIBRARY`

**Пример:**
```
OWNER(g:my_group)
PY3_LIBRARY()
    PEERDIR(
        my/lib/py23
        my/lib/cpp
        my/lib/proto
    )

    PY_SRCS(x.py)
END()
```

{% note info %}

В силу особенностей сборки при непосредственной сборке такой библиотеки командой `ya make path/to/lib` у неё может совсем не быть артефакта.
Результатом может быть объектный файл, который непосредственно влинкуется в программу.

{% endnote %}


## PY2_PROGRAM


`PY2_PROGRAM([name])`


Программа на Python 2. Если указан `name`, то имя файла программы будет `name`, иначе - последний слог пути к модулю программы.
**Макрос устарел. Используйте вместо него макрос [`PY3_PROGRAM`](#py3_program) (подробнее см. тут: https://clubs.at.yandex-team.ru/arcadia/23044).**

- Собирается в исполняемый файл под целевую платформу. Исполняет в интерпретаторе модуль отмеченный как main (`__main__.py` если указан в [`PY_SRCS`](macros.md#py_srcs), то что помечено как MAIN в  [`PY_SRCS`](macros.md#py_srcs) или в макросе [`PY_MAIN`](macros.md#py_main))

- Включает все библиотеки, от которых транзитивно зависит, и интерпретатор Python 2. Настраивает пути поиска `Python.h` на Python 2 из Аркадии.

- При `PEERDIR` выбирает Python 2 вариант из [`PY23_LIBRARY`](#py23_library) и `PROTO_LIBRARY`. Может зависеть по `PEERDIR` только от Python 2 модулей.

Можно в | Нельзя в
:--- | :---
`PY2_LIBRARY` | `PY3_LIBRARY`
`LIBRARY` | `LIBRARY` c `USE_PYTHON3()`
`PY23_LIBRARY` | -
- | `PY2MODULE`, `PY_ANY_MODULE`, `PY23_NATIVE_LIBRARY`
- | `PY2_PROGRAM`, `PY2TEST`, `PY3_PROGRAM`, `PY3TEST`, `PY23_TEST`

**Пример:**
```
OWNER(g:my_group)
PY2_PROGRAM()
    PEERDIR(
        my/lib/py23
        my/lib/cpp
        my/lib/proto
    )

    PY_SRCS(
       x.py
       __main__.py
     )
END()
```


## PY3_PROGRAM


`PY3_PROGRAM([name])`


Программа на Python 3. Если указан `name`, то имя файла программы будет `name`, иначе - последний слог пути к модулю программы.

- Собирается в исполняемый файл под целевую платформу. Исполняет в интерпретаторе модуль, отмеченный как main (`__main__.py`, если указан в [`PY_SRCS`](macros.md#py_srcs);
то, что помечено как MAIN в  [`PY_SRCS`](macros.md#py_srcs) или в макросе [`PY_MAIN`](macros.md#py_main))

- Включает все библиотеки, от которых транзитивно зависит, и интерпретатор Python 3. Настраивает пути поиска `Python.h` на Python 3 из Аркадии.

- При `PEERDIR` выбирает Python 3 вариант из [`PY23_LIBRARY`](#py23_library) и `PROTO_LIBRARY`. Может зависеть по `PEERDIR` только от Python 2 модулей.

Можно в | Нельзя в
:--- | :---
`PY3_LIBRARY` | `PY2_LIBRARY`
`LIBRARY` c `USE_PYTHON3()` | `LIBRARY`
`PY23_LIBRARY` | -
- | `PY2MODULE`, `PY_ANY_MODULE`, `PY23_NATIVE_LIBRARY`
- | `PY2_PROGRAM`, `PY2TEST`, `PY3_PROGRAM`, `PY3TEST`, `PY23_TEST`

**Пример:**
```
OWNER(g:my_group)
PY2_PROGRAM()
    PEERDIR(
        my/lib/py23
        my/lib/cpp
        my/lib/proto
    )

    PY_SRCS(
       x.py
       __main__.py
     )
END()
```


## PY23_LIBRARY


`PY23_LIBRARY()`


Мультимодуль, собирающий библиотеку для Python 2 или Python 3 в зависимости от того, из какого модуля он используется.


`PY23_LIBRARY` позволяет собрать два разных варианта модуля и использовать необходимый в зависимости от модуля из которого на него поставлен `PEERDIR`.
Такая конструкция называется *мультимодулем*. `PY23_LIBRARY` умеет строиться как [`PY2_LIBRARY`](#py_library) и как [`PY3_LIBRARY`](#py3_library).
Макросы [`PY_SRCS`](macros.md#py_srcs), [`PY_NAMESPACE`](macros.md#py_namespace) и [`PY_REGISTER`](macros.md#py_register) будут обрабатываться в соответствии с вариантом, который строится.

Откуда PEERDIR | Чем станет
:--- | :---
`PY2_LIBRARY`, `PY2_PROGRAM`, `PY2TEST` | `PY2_LIBRARY`
`PY3_LIBRARY`, `PY3_PROGRAM`, `PY3TEST` |  `PY3_LIBRARY`
`LIBRARY`, `PROGRAM` |  `PY2_LIBRARY`
`LIBRARY`, `PROGRAM`  c `USE_PYTHON3()` | `PY3_LIBRARY`
`PY23_LIBRARY` | наследуется `PY2_LIBRARY` или `PY3_LIBRARY`
`PY23_TEST` | `PY2_LIBRARY` и `PY3_LIBRARY` в соответствующих вариантах теста


`PY23_LIBRARY` используется так же, как `PY2_LIBRARY`, например:

```
OWNER(g:my_group)
PY23_LIBRARY()
    PEERDIR(
        my/lib/py23
        my/lib/cpp
        my/lib/proto
    )

    PY_SRCS(x.py)
END()
```

{% note info %}

В силу особенностей сборки при непосредственной сборке такой библиотеки командой `ya make path/to/lib` у неё может совсем не быть артефакта.
Результатом может быть объектный файл, который непосредственно влинкуется в программу.

{% endnote %}


## PY23_NATIVE_LIBRARY


`PY23_NATIVE_LIBRARY()`


Мультимодуль для переиспользования С/C++ кода между 2м и 3м питонами без зависимости от Аркадийного питона. Такая библиотека может
использовать функции из `Python.h` для 2го или 3го питона в зависимости от контекста, но не принесёт runtime-библиотеку Python из
Аркадии в линковку, а значит её можно использовать как из `PY2MODULE` (которому библиотека не нужна), так и из
[`PY2_PROGRAM`](#py_program) / [`PY3_PROGRAM`](#py3_program) /[`PY2_LIBRARY`](#py_library) / [`PY3_LIBRARY`](#py3_library) / [`PY23_LIBRARY`](#py23_library), которые приносят зависимость на нужную библиотеку.

{% note warning %}

`PY23_NATIVE_LIBRARY`, также как и из [`PY2MODULE`](#pymodule) запрещено иметь `PEERDIR` на [`PY2_LIBRARY`](#py_library) / [`PY3_LIBRARY`](#py3_library) / [`PY23_LIBRARY`](#py23_library), а также содержать макрос [`PY_SRCS`](macros.md#py_srcs).

{% endnote %}

**Пример:**
```
OWNER(g:my_group)
PY23_NATIVE_LIBRARY()
    PEERDIR(
        # my/lib/py23 -- this is prohibited
        my/lib/cpp
        my/lib/proto  # This will be C++ dependency
    )

    SRCS(py_mylib.cpp)

    PY_REGISTER(mylib)
END()
```


## PY2MODULE { #pymodule }


`PY2MODULE(name major_ver [minor_ver] [EXPORTS symlist_file] [PREFIX prefix])`
`PY3MODULE(name major_ver [minor_ver] [EXPORTS symlist_file] [PREFIX prefix])`

Динамическая библиотека, загружаемая во внешний интерпретатор Python2 или Python3 как расширение.
Параметры модуля такие же, как у модуля `DLL`.

- `name` - основное имя библиотеки
- `major_ver` и `minor_ver` - часть версии библиотеки, попадающая в её имя. Оба параметра должны быть целыми числами.
- `EXPORTS` - файл с описанием экспортов библиотеки, может быть также задан отдельно макросом `EXPORT_SCRIPT`
- `PREFIX` - изменить префикс имени библиотеки, по умолчанию используется lib

С++ или Cython код пишется в макросе `SRCS`.

`PY2MODULE`/`PY3MODULE` может содержать определение только одного Python-модуля (не более одного `.pyx` в `SRCS`). При этом в сборку `PY2MODULE` и `PY3MODULE` 
не должно попадать Аркадийной сборки Python, поскольку она встраивает Python, а `PY2MODULE`/`PY3MODULE`` должен использоваться именно с внешним.

Для использования библиотеку надо разместить на файловой системе так, чтобы внешний интерпретатор Рython её нашёл.

{% note warning %}

`PY2MODULE`/`PY3MODULE` не должен иметь `PEERDIR` на [`PY2_LIBRARY`](#py_library) / [`PY3_LIBRARY`](#py3_library) / [`PY23_LIBRARY`](#py23_library), а также содержать макрос [`PY_SRCS`](macros.md#py_srcs).

{% endnote %}

`PY2MODULE` - это С++ модуль для Python 2. Его взаимодействие с мультимодулями описано ниже

PEERDIR | Вариант
:--- | :---
`PY23_NATIVE_LIBRARY` | Python 2
`PROTO_LIBRARY` | C++
`PY23_LIBRARY` | **запрещено**

`PY3MODULE` - это С++ модуль для Python 3. Его взаимодействие с мультимодулями описано ниже

PEERDIR | Вариант
:--- | :---
`PY23_NATIVE_LIBRARY` | Python 3
`PROTO_LIBRARY` | C++
`PY23_LIBRARY` | **запрещено**


**Пример:**

```
OWNER(g:my_group)
PY3MODULE(mywraoper)
   PEERDIR(proj/lib/mycode)
   SRCS(wrap.cpp)
END()
```


## PY_ANY_MODULE


`PY2MODULE(name major_ver [minor_ver] [EXPORTS symlist_file] [PREFIX prefix])`


Динамическая библиотека, загружаемая во внешний интерпретатор Python как расширение.
Версия Python выбирается макросом, который может стоять под условием. Параметры модуля такие же, как у модуля `DLL`.

- `name` - основное имя библиотеки
- `major_ver` и `minor_ver` - часть версии библиотеки, попадающая в её имя. Оба параметра должны быть целыми числами.
- `EXPORTS` - файл с описанием экспортов библиотеки; может быть также задан отдельно макросом `EXPORT_SCRIPT`
- `PREFIX` - изменить префикс имени библиотеки; по умолчанию используется lib


С++ или Cython код пишется в макросе `SRCS`. В такой библиотеке макросами [`PYTHON2_MODULE()` и `PYTHON3_MODULE()`](macros.md#python_module) можно выбрать для какой версии Python нужно собирать модуль.

`PY_ANY_MODULE` может содержать определение только одного python-модуля (не более одного `.pyx` в `SRCS`). При этом в сборку `PY_ANY_MODULE`
не должно попадать Аркадийной сборки Python, поскольку она встраивает Python, а `PY_ANY_MODULE` должен использоваться именно с внешним.

Для использования библиотеку надо разместить на файловой системе так, чтобы внешний интерпретатор Рython её нашёл.

Чтобы выбрать версию питона для `PY_ANY_MODULE` нужно использовать макросы [`PYTHON2_MODULE` и `PYTHON3_MODULE`](macros.md#python_module).

{% note warning %}

`PY_ANY_MODULE` не должен иметь `PEERDIR` на [`PY2_LIBRARY`](#py_library) / [`PY3_LIBRARY`](#py3_library) / [`PY23_LIBRARY`](#py23_library), а также содержать макрос [`PY_SRCS`](macros.md#py_srcs).

{% endnote %}

`PY_ANY_MODULE` - это С++ модуль для Python, версия которого выбирается макросом `PYTHONx_MODULE`. Его взаимодействие с мультимодулями описано ниже

PEERDIR | Вариант
:--- | :---
`PY23_NATIVE_LIBRARY` | Python2 или Python3 в зависимости от настройки `PYTHONx_MODULE`
`PROTO_LIBRARY` | C++
`PY23_LIBRARY` | **запрещено**

**Пример:**

```
OWNER(g:my_group)
PY_ANY_MODULE(mywraoper)
   # Use -DPYTHON_CONFIG or -DUSE_SYSTEM_PYTHON to define Python version
   IF (PYTHON_CONFIG MATCHES "python3" OR USE_SYSTEM_PYTHON MATCHES "3.")
       PYTHON3_MODULE()
   ELSE()
       PYTHON2_MODULE()
   ENDIF()

   SRCS(mywrapper.pyx)
END()
```


## PY2TEST / PY3TEST { #pytestpy3test }


`PY2TEST()`

`PY3TEST()`

Тесты на фреймворке `pytest` для Python 2 и Python 3 соответственно.

Собирает (по команде `ya make`) и запускает (по команде `ya make -t`) тестовую сюиту, в которую попадают все функции вида `test_.*` из всех файлов, перечисленные в макросах
[`TEST_SRCS`](macros.md#test_srcs), как в самом модуле, так и во всех [`PY2_LIBRARY`](#py_library) / [`PY3_LIBRARY`](#py3_library) / [`PY23_LIBRARY`](#py23_library),
достижимых из этого модуля по `PEERDIR`.

Внутри модуля можно писать как макросы для сборки, такие как [`PY_SRCS`](macros.md#py_srcs), `PEERDIR` и т.п., так и макросы для запуска тестов `DEPENS`, `DATA` и т.п.

О тестировании `ya make` можно прочитать здесь:

- [Общие сведения](../tests/index.md)
- [О тестах на Python](../tests/python.md)

Тесты собираются в исполняемую программу как [`PY2_PROGRAM`](#py_program) / [`PY3_PROGRAM`](#py3_program), однако исполнить эту программу может только сама система сборки.
Как следствие этого в тесты собираются все их зависимые библиотеки.

Правила `PEERDIR` для `PY2TEST` и `PY3TEST` такие же как у [`PY2_PROGRAM`](#py_program) и [`PY3_PROGRAM`](#py3_program) соответственно.

Правила для `PY2TEST`:

Можно в | Нельзя в
:--- | :---
`PY2_LIBRARY` | `PY3_LIBRARY`
`LIBRARY` | `LIBRARY` c `USE_PYTHON3()`
`PY23_LIBRARY` | -
- | `PY2MODULE`, `PY_ANY_MODULE`, `PY23_NATIVE_LIBRARY`
- | `PY2_PROGRAM`, `PY2TEST`, `PY3_PROGRAM`, `PY3TEST`, `PY23_TEST`

Правила для `PY3TEST`:

Можно в | Нельзя в
:--- | :---
`PY3_LIBRARY` | `PY2_LIBRARY`
`LIBRARY` c `USE_PYTHON3()` | `LIBRARY`
`PY23_LIBRARY` | -
- | `PY2MODULE`, `PY_ANY_MODULE`, `PY23_NATIVE_LIBRARY`
- | `PY2_PROGRAM`, `PY2TEST`, `PY3_PROGRAM`, `PY3TEST`, `PY23_TEST`

{% note info %}

Правила выше не распространяются на зависимость `DEPENDS`. Макрос `DEPENDS` позволяет использовать построенную программу для исполнения в тесте. Тесты на любой версии Python могут использовать программы на любой версии Python,
поскольку Python-программы, собираемые ya.make самодостаточны. Так `PY2TEST` может использовать как [`PY2_PROGRAM`](#py_program), так и [`PY3_PROGRAM`](#py3_program), для `PY3TEST` - аналогично.

{% endnote %}

**Пример**

`ya.make`:

{% code '/devtools/examples/tutorials/python/lib_with_test/test/ya.make' %}

Код теста:

{% code '/devtools/examples/tutorials/python/lib_with_test/test/test_decorate.py' lang='python' %}


## PY23_TEST

`PY23_TEST()`

Тесты на фреймворке `pytest` для Python 2 и Python 3 одновременно.

`PY23_TEST` является мультимодулем и по сути является одновременным описанием [`PY2TEST` и `PY3TEST`](#pytestpy3test) в одном `ya.make`.

Из него обирается (по команде `ya make`) и запускается (по команде `ya make -t`) тестовые сюиты, в которые попадают все функции вида `test_.*` из всех файлов, перечисленных в макросах
[`TEST_SRCS`](macros.md#test_srcs), как в самом модуле, так и во всех [`PY2_LIBRARY`](#py_library) / [`PY3_LIBRARY`](#py3_library) / [`PY23_LIBRARY`](#py23_library),
достижимых из этого модуля по `PEERDIR`. [`PY2TEST`](#pytestpy3test)-вариант собирает все зависимости для Python 2, а [`PY3TEST`](#pytestpy3test) - для Python 3. `ya make -t` по умолчанию запускает оба варианта тестов.

Запуск версий таких тестов можно контролировать из командной строки, дописав следующие ключи в командную строку `ya make -t`:

- `--test-type=pytest` запускает `py2test` и `py3test`
- `--test-type=py2test` запускает только `py2test`
- `--test-type=py3test` запускает только `py3test`

Внутри модуля можно писать как макросы для сборки такие как `PY_SRCS`(macros.md#py_srcs), `PEERDIR` и т.п., так и макросы для запуска тестов `DEPENDS`, `DATA` и т.п.
Выбор зависимостей осуществляется для каждого варианта отдельно в соответствии с правилами для [`PY2TEST` и `PY3TEST`](#pytestpy3test) соответственно.

О тестировании можно прочитать здесь:

- [О тестировании в ya make](../tests/index.md)
- [О тестах на Python](../tests/python.md)

Python 2 и Python 3 варианты тестов в `PY23_TEST` являются отдельными сюитами - их результаты лежат в разных поддиректориях директории `test-result` и у них отдельные [канонические данные](../tests/canon.md).
Так же, как при обычном запуске, при канонизации `ya make -tZ` будут запущены и канонизированы оба варианта тестов.

**Пример**

```
OWNER(g:yatool)

PY23_TEST()

TEST_SRCS(
    platform_matcher_test.py
)

PEERDIR(
    devtools/ya/yalibrary/platform_matcher
)

DATA(
    arcadia/autocheck
)

END()
```
