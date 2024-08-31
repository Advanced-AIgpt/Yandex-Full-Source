# Python : макросы


Макросы для сборки Python:
- [PY_SRCS](#py_srcs)
- [ALL_PY_SRCS](#all_py_srcs)
- [TEST_SRCS](#test_srcs)
- [ALL_PY_SRCS](#all_pytest_srcs)
- [PY_MAIN](#py_main)
- [PY_REGISTER](#py_register)
- [PY_NAMESPACE](#py_namespace)
- [USE_PYTHON2 / USE_PYTHON3](#use_python)
- [PYTHON2_ADDINCL / PYTHON3_ADDINCL](#python_addincl)
- [PYTHON2_MODULE / PYTHON3_MODULE](#python_module)
- [BUILDWITH_CYTHON_CPP / BUILDWITH_CYTHON_C](#buildwith_cython)


## PY_SRCS

`PY_SRCS([CYTHON\_C] [SWIG_C] [TOP_LEVEL | NAMESPACE ns] [MAIN main_file] Files...)`

Собирает исходные файлы с помощью Аркадийной бинарной сборки Python. В конечном итоге прекомпилированный и исходный код попадает
в программу вместе с интерпретатором которому они доступны как обычные модули Python.

Имена модулей формируются либо из путей от корня Аркадии, либо в соответствии с параметрами `TOP_LEVEL` и `NAMESPACE`. `__init__.py` не требуется,
но если указан, то будет использован при импорте пакета, содержащего `__init__.py`.

`TOP_LEVEL` - делает пакет доступным без пути, на верхнем уровне.

`NAMESPACE` - заменяет в пакете префикс, выводимый из пути, на заданный параметром `NAMESPACE`

{% note alert "Ограничение на использование TOP_LEVEL и NAMESPACE" %}

Запрещено использовать `TOP_LEVEL` и `NAMESPACE` если на это нет веских причин (вы выкладываетесь в опенсорс или у вас есть потребители за пределами Аркадии). Их наличие осложняет понимание того, где находится код, по импортам, осложняет рефакторинг кода, и допускает вероятность пересечения разных модулей по именам.

{% endnote %}

Пусть у нас в директории `x/y` есть файлы `a.py`, `z/b.py` и `__init__.py`. Мы хотим записать их в макрос
```
PY_SRCS(
    a.py
    z/b.py
    __init.py
)
```
Влияние `TOP_LEVEL` и `NAMESPACE` на импорты этих файлов приведено в таблице.

В `PY_SRCS()` | `a.py` | `z/b.py` | `__init__.py`
:--- | :--- | :--- | :---
Путь в Аркадии | `x/y/a.py` | `x/y/z/b.py` | `x/y/__init__.py`
Default | `import x.y.a` | `import x.y.z.b` | `import x.y`
`TOP_LEVEL` | `import a` | `import z.b` | **запрещено**
`NAMEPASE n.s` | `import n.s.a` | `import n.s.z.b` | `import n.s`

PY_SRCS поддерживает код Python (файлы `*.py`), файлы protobuf (`*.proto` и `*.ev`) - из них генерируется код Python, файлы Flatbuffers (`.fbs`), Cython-файлы (`*.pyx`) с автоматическим подключением одноимённых `*.pxd`, и SWIG-файлы (`*.swg`). Cython и SWIG генерируют расширения, которые автоматически регистрируются как встроенные модули Python в соответствии с путями и параметрами `TOP_LEVEL`и `NAMESPACE`.

По умолчанию из Cython-файлов и SWIG-файлов генерируется С++-код. Чтобы переключить для них режим генерации в С используйте параметры `CYTHON_C` и `SWIG_C` соответственно.

`MAIN` - модуль, который будут запущен на старте программы. Имеет смысл только в модулях [`PY2_PROGRAM`](modules.md#py_program)

Когда не указан параметр `MAIN` и макрос [`PY_MAIN`](py_main), выполнение начнётся с кода в файле `__main__.py`.
`__main__` всегда является `TOP_LEVEL`, поэтому другие файлы того же проекта из `__main__` можно импортировать только по абсолютному пути.


**Примеры:**
```
    OWNER(g:grp)
    PY23_LIBRARY(mymodule)
        PY_SRCS(a.py sub/dir/b.py e.proto sub/dir/f.proto c.pyx sub/dir/d.pyx g.swg sub/dir/h.swg)
    END()
```

```
    OWNER(g:grp)
    PY3_LIBRARY(mymodule2)
        PY_SRCS(
            CYTHON_C
            NAMESPACE my.name.space
            a.py
            c.pyx
            __init__.py
        )
    END()
```


`PY_SRCS` может быть использован во всех модулях бинарной сборки Python. [`PY*_LIBRARY`, `PY*_PROGRAM`, `PY*TEST`](modules.md).

`PY_SRCS` работает в соответствии с версией Python модуля в котором написан.

Допускается, но настоятельно не рекомендуется, использование `PY_SRCS` в `PROGRAM` и `LIBRARY`.

{% note alert %}

Никогда не используйте `PY_SRCS` в `LIBRARY` если вы планируете использовать эту библиотеку из модулей расширения Python (`PY2MODULE`/`PY_ANY_MODULE`).

{% endnote %}


## ALL_PY_SRCS

`ALL_PY_SRCS([TOP_LEVEL | NAMESPACE ns] [RECURSIVE] [Dirs...])`

Макрос позволяет получить все .py-файлы в текущей или перечисленных директориях и обработать их как [PY_SRCS](#py_srcs).

- Параметры `TOP_LEVEL` и `NAMESPACE`. Обрабатываются также, как в [PY_SRCS](#py_srcs). Также как и в обычном [PY_SRCS](#py_srcs) специально обрабатывается файл `__main__.py`
- Параметр `RECURSIVE` делает сбор файлов рекурсивным от текущей или указанных директорий.
  
  {% note alert %}

  Запрещено указывать пути поиска, в которых есть другие файлы `ya.make`. Каждый исходный файл должен принадлежать одному модулю.
  Макрос реализован таким образом, чтобы вместе с .py в [PY_SRCS](#py_srcs) попадали все файлы ya.make, кроме того, где написан сам макрос. [PY_SRCS](#py_srcs) отвергает эти файлы с ошибкой.

  {% endnote %}

{% note warning %}

В одном `ya.make` можно написать только один такой макрос. Но с обычными [PY_SRCS](#py_srcs) он соседствовать может.

{% endnote %}

Примеры использования макроса можно посмотреть [в этих проектах](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/dummy_arcadia/py_all).


## TEST_SRCS

`TEST_SRCS(Files...)`

Добавляет `*.py`-файлы к сборке Python-тестов (`PY*TEST`) и Python-библиотек (`PY2_LIBRARY`) аналогично макросу [`PY_SRCS`](#py_srcs).
В дополнение к этому в файлах перечисленных в `TEST_SRCS` тестовый фреймворк будет искать тесты как функции `test_.*`.


{% note alert %}

У `TEST_SRCS` нет параметров `NAMESPACE`, `TOP_LEVEL` и `MAIN`. Не кладите туда не `*.py`-файлы, а также файлы `__main__.py`.

{% endnote %}

- [Про тесты на Python](../tests/python)

## ALL_PYTEST_SRCS

`ALL_PYTEST_SRCS([RECURSIVE] [Dirs...])`

Макрос позволяет получить все .py-файлы в текущей или перечисленных директориях и обработать их как [TEST_SRCS](#test_srcs).

- Параметр `RECURSIVE` делает сбор файлов рекурсивным от текущей или указанных директорий.
  
  {% note alert %}

  Запрещено указывать пути поиска, в которых есть другие файлы `ya.make`. Каждый исходный файл должен принадлежать одному модулю.
  Макрос реализован таким образом, чтобы вместе с .py в [TEST_SRCS](#test_srcs) попадали все файлы ya.make, кроме того, где написан сам макрос. [TEST_SRCS](#test_srcs) отвергает эти файлы с ошибкой.

  {% endnote %}

{% note warning %}

В одном `ya.make` можно написать только один такой макрос. Но с обычными [TEST_SRCS](#test_srcs) он соседствовать может.

{% endnote %}

## PY_MAIN

`PY_MAIN(pkg.module[:function])`

В модулях [`PY2_PROGRAM`](modules.md#py_program) задаёт функцию, с которой начнётся исполнение программы.

Если функция не указана, предполагается функция main (`pkg.module.main`), если указана, то будет вызвана `pkg.module.function`.

Функцию `main` в модуле, указанном в [`PY_SRCS`](#py_srcs), можно также сделать главной, указав этот модуль в параметре MAIN.

Когда не указан `PY_MAIN` и параметр `MAIN`, выполнение начнётся с кода в файле `__main__.py`.
`__main__` всегда является TOP_LEVEL, поэтому другие файлы того же проекта из `__main__` можно импортировать только по абсолютному пути.

**Примеры:**
```
OWNER(g:grp)
PY2_PROGRAM(prog)
    PY_MAIN(prog)
    PY_SRCS(
        TOP_LEVEL
        prog.py
    )
END()
```

```
OWNER(g:grp)
PY3_PROGRAM()
    PEERDIR(base/some/lib)
    PY_MAIN(base.some.lib.entry:entry)
END()
```

```
OWNER(g:grp)
PY3_PROGRAM(prog)
    PY_SRCS(
        MAIN key_generator.py
    )
END()
```

```
OWNER(g:grp)
PY3_PROGRAM(prog)
    PY_SRCS(
        __main__.py
    )
END()
```


## PY_REGISTER

`PY_REGISTER([package.]module_name[=)`

Python знает о том, какие встроенные модули можно импортировать, благодаря их регистрации при сборке или при старте интерпретатора. Все модули из исходников, перечисленных в PY_SRCS,
регистрируются автоматически. Чтобы зарегистрировать модули на С/C++ из обычной С++ сборки через `SRCS` нужно воспользоваться `PY_REGISTER`.


`PY_REGISTER(module_name)` укажет питону, что для загрузки модуля module_name нужно вызвать C-функцию `init<module_name>` (в Python 2) или `PyInit_<module_name>` (в Python 3).


`PY_REGISTER(package.module_name)` укажет питону, что для загрузки модуля `module_name` в пакете package нужно вызвать C-функцию `init<X><package_part>*<Y><module_name>`, переопределит имена
`init<module_name>` (в Python 2) или `PyInit_<module_name>` (в Python 3) через флаг компилятора, чтобы не приходилось менять имя в исходниках. Так сделано, чтобы одноимённые модули
в разных пакетах имели разные C-имена и их можно было поместить в один и тот же исполняемый файл.

* `X` - это длина имени пакета, `Y` - длина имени модуля, например `init2my7package11module_name` для `my.package.module_name`

**Примеры:**


```
PY2_LIBRARY()

PY_REGISTER(my_module)
SRCS(
    main.cpp
)
END()


#include <Python.h>

PyMODINIT_FUNC initmy_module() { // This will be called for my_module load
// ...
}
```


```
PY2_LIBRARY()

PY_REGISTER(my.pkg.my_module)
SRCS(
    main.cpp
)
END()
```

main.cpp:

```
#include <Python.h>

PyMODINIT_FUNC initmy_module() { // This will become init2my3pkg10module
// ...
}

```

## PY_NAMESPACE

`PY_NAMESPACE(prefix)`

Устанавливает префикс для всех исходных файлов модуля. Аналогично параметру `NAMESPACE` макроса [`PY_SRCS`](#py_srcs).
Может применяться в `PROTO_LIBRARY`, где `SRCS` общий для всех языков и потому параметр `NAMESPACE` недоступен.

**Пример (tools/home/mylibrary/proto):**

```
OWNER(g:owners)
PROTO_LIBRARY()

PY_NAMESPACE(
    mylibrary.proto
)

SRCS(
    config.proto
)
END()
```


## USE_PYTHON2 / USE_PYTHON3 { #use_python }

`USE_PYTHON2()`

`USE_PYTHON3()`

Добавляет зависимость на библиотеку Python2 или Python3 к программе или библиотеке на C++ и делает её совместимой с Python2/Python3.
Совместимость означает правильный путь поиска `<Python.h>` (`ADDINCL`) и выбор правильной версии при зависимостях (`PEERDIR`) от [`PY23_LIBRARY`](modules.md#py23_library).

- Если вы хотите использовать `#include <Python.h>` с Python2, используйте `USE_PYTHON2()` а лучше сделайте свою библиотеку `PY2_LIBRARY`.
- Если вы хотите использовать `#include <Python.h>` с Python3, используйте `USE_PYTHON3()` а лучше сделайте свою библиотеку `PY3_LIBRARY`.
- Если вы хотите использовать `#include <Python.h>` и с Python2 и с Python3, используйте сделайте свою библиотеку `PY23_LIBRARY`.

{% note alert %}

Никогда не используйте `USE_PYTHON2/USE_PYTHON3` c `PY2_PROGRAM`, `PY2_LIBRARY` и т.п. Во всех этих модулях уже есть необходимые зависимости и пути поиска заголовков.

{% endnote %}


## PYTHON2_ADDINCL / PYTHON3_ADDINCL { #python_addincl }

`PYTHON2_ADDINCL()`

`PYTHON3_ADDINCL()`

Эти макросы добавляют соответствующие заголовки Python в пути поиска заголовков без добавления зависимости на библиотеку Python.

Есть только две причины использовать эти макросы:
1. В `LIBRARY` или [`PY23_NATIVE_LIBRARY`](./modules.md#py23_native_library), используемой из [`PYxMODULE`](modules.md#pymodule). Поскольку [`PYxMODULE`](modules.md#pymodule) компилируется с внешним Python, то ему нельзя иметь зависимость на Аркадийный Python, а заголовки могут быть нужны.
2. В системных библиотеках самого Python, поскольку `PEERDIR` в них создаст цикл по зависимостям.

Во всех остальных случаях используйте [`USE_PYTHON2`/`USE_PYTHON3`](#use_python)

{% note alert %}

Никогда не используйте `PY2_PROGRAM`, `PY2_LIBRARY` и т.п. Во всех этих модулях уже есть необходимые зависимости и пути поиска заголовков.

{% endnote %}


## PYTHON2_MODULE / PYTHON3_MODULE { #python_module }

`PYTHON2_MODULE()`

`PYTHON3_MODULE()`

Используется для выбора версии Python в модуле - расширении для внешнего Python [`PY_ANY_MODULE`](modules.md#py_any_module).

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

## BUILDWITH_CYTHON_CPP / BUILDWITH_CYTHON_C { #build_with_cython }

`BUILDWITH_CYTHON_CPP()`

`BUILDWITH_CYTHON_C()`

Генерировать из `.pyx`-файлов C++ или С код. Аналогично параметру `CYTHON_C` макроса [`PY_SRCS`](#py_srcs)

