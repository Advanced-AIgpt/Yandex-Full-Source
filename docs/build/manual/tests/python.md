# Тесты на Python

{% note info %}

Пример проекта на Python с использованием внешних зависимостей можно найти [здесь](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tests/using_py_library).

{% endnote %}

Основным фреймворком для написания тестов на Python является [pytest](https://pytest.org/). Поддерживаются Python 2 (макрос `PY2TEST`) и Python 3 (макрос `PY3TEST`). Типичный файл **ya.make** выглядит так:

```yamake
OWNER(g:my-group)

PY3TEST() # Используем pytest для Python 3 (PY2TEST будет означать Python 2)

PY_SRCS( # Зависимости тестов, например, абстрактный базовый класс теста
    base.py
)

TEST_SRCS( # Перечисление всех файлов с тестами
    test.py
)

SIZE(MEDIUM)

END()
```

Подробное описание всех возможностей фреймворка **pytest** можно почитать в [документации](https://docs.pytest.org/). Простейший тест выглядит следующим образом:

```python
# test.py
import logging
import pytest

logger = logging.getLogger("test_logger")

def test_one_1():
    logger.info("Info message")

def test_one_2():
    assert 1 == 2
```

Ниже перечислены некоторые особенности, которые следует учитывать при написании кода тестов:

* Имена тестовых классов должны начинаться на `Test`, в них не должно быть конструктора `__init__`.
* Имена тестовых методов / функций должны начинаться на `test`.
* pytest умеет показывать расширенную информацию при срабатывании assert, поэтому писать подробное сообщение об ошибке для assert обычно не надо.
* Если тесты рассчитывают на файл `base.py` или код в `__init__.py`, то их нужно явно перечислить в макросе `TEST_SCRS` или `PY_SRCS` (см. пример ya.make выше).
* Переменная [sys.executable](https://docs.python.org/3/library/sys.html#sys.executable), ссылается не на интерпретатор Python, а на исполняемый файл теста. Чтобы получить путь до интерпретатора, необходимо вызвать метод `yatest.common.python_path()`:
```python
import pytest
import yatest.common

def test_python_path():
    python_path = yatest.common.python_path()
    # ...
```
* Переменная тестового модуля `__file__` в общем случае не будет указывать на реальное местоположение файла с тестом. Подробнее о том, как получить путь до файла, описано в следующем разделе.

## Библиотека yatest { #yatest }

В едином репозитории код на Python (в том числе и код тестов) перед запуском собирается в исполняемую программу. Для того, чтобы работать с файлами, внешними программами, сетью и так далее, следует использовать специальную библиотеку [yatest](https://a.yandex-team.ru/arc/trunk/arcadia/library/python/testing/yatest_common). Некоторые полезные методы этой библиотеки приведены в таблице:

#### Работа с runtime окружением теста { #runtime-fs }
Метод | Описание
:--- | :---
`yatest.common.source_path` | Возвращает путь до файла от корня единого репозитория. Путь должен быть перечислен в макросе `DATA` в **ya.make**, начинаясь с `arcadia/`.
`yatest.common.test_source_path` | Возвращает путь до файла относительно расположения теста в репозитории.
`yatest.common.binary_path` | Возвращает путь до собранной программы, от которой зависит тест. Программа должна быть перечислена в секции `DEPENDS` в **ya.make** у теста. В качестве аргумента указывается путь, включающий имя файла программы.
`yatest.common.build_path` | Возвращает путь от корня сборочной директории для зависимостей теста.
`yatest.common.data_path` | Возвращает путь от корня каталога [arcadia_tests_data](https://a.yandex-team.ru/arc/trunk/arcadia_tests_data). Путь должен быть перечислен в макросе `DATA` в **ya.make** у теста и начинаться с `arcadia_tests_data/`.
`yatest.common.work_path` | Возвращает путь до рабочей директории теста, где можно сохранять временные данные.
`yatest.common.output_path` | Возвращает путь до директории `testing_out_stuff`. Данные сохранённые внутри неё будут доступны после тестирования.
`yatest.common.test_output_path` | Возвращает путь от директории `testing_out_stuff/<test_name>/path`. Директория `testing_out_stuff/<test_name>` будет создана автоматически.
`yatest.common.ram_drive_path` | Возвращает путь от RAM диска, если он предоставлен окружением. На Linux можно использовать опцию `--private-ram-drive` для предоставления индивидуального RAM диска для тестового узла, если он его заказывает с помощью `REQUIREMENTS(ram_disk:X)`, где X размер в GiB
`yatest.common.output_ram_drive_path` | Возвращает путь до уникальной созданной директории внутри RAM диска, контент которой будет перемещена в `testing_out_stuff/ram_drive_output` после тестирования.

#### Вспомогательные методы { #helpers }
Метод | Описание
:--- | :---
`yatest.common.execute` | Запускает внешнюю программу. В случае падения программы по сигналу, автоматически сохраняет core dump file, получает backtrace и привязывает эти данные к тесту.
`yatest.common.get_param` | Получить значение параметра, переданного через командную строку (`ya make -tt --test-param key=value`).
`yatest.common.network.get_port` | Получить указанный или произвольный свободный сетевой порт.
`yatest.common.python_path` | Возвращает путь до python binary.
`yatest.common.gdb_path` | Возвращает путь до gdb binary.
`yatest.common.java_bin` | Возвращает путь до java binary. В тесте требуется `DEPENDS(jdk)`.

#### Работа с каноническими данными { #canonization }
Метод | Описание
:--- | :---
`yatest.common.canonical_file` | Позволяет создать объект канонического файла, который можно вернуть из теста для его канонизации. Можно возвращать списки canonical_file, порядок имеет значение.
`yatest.common.canonical_dir` | Позволяет канонизировать директории целиком.
`yatest.common.canonical_execute` | Формирует канонический файл на основе stdout от запуска команды.
`yatest.common.canonical_py_execute` | Формирует канонический файл на основе stdout от запуска python-скрипта.

Пример получения доступа к данным из репозитория:

```python
import pytest
import yatest.common

def test_path():
    # Путь до файла из единого репозитория
    script_file = yatest.common.source_path('my-project/utils/script.sh')
    # Путь до каталога data рядом с тестом
    data_dir = yatest.common.test_source_path('data')
```

Получение доступа к тестовым параметрам:

```python
import pytest
import yatest.common

def test_parameters():
    # Получаем значение тестового параметра
    test_username = yatest.common.get_param('username')
```

Получение сетевого порта:

```python
import pytest
from yatest.common import network

def test_network_port():
    with network.PortManager() as pm:
        port = pm.get_port() # Свободный порт
```

Выполнение внешней программы:

```python
import pytest
import yatest.common

def test_execute():
    # Запускаем внешнюю команду
    p = yatest.common.execute(
        [ 'echo', 'hello, world!'],
        check_exit_code=False
    )
    code = p.exit_code
```

Канонизация файла в репозиторий с использованием внешнего diff_tool

```python
def test_canon():
    diff_tool = yatest.common.binary_path("path/to/diff_tool")
    with open("1.txt", "w") as afile:
        afile.write("canon data\n")
    return yatest.common.canonical_file("1.txt", diff_tool=[diff_tool, "--param1", "1", "--param2", "2"], local=True)
```

## Импорт-тесты { #import }

Для кода на Python при [ручном](../../usage/ya_make/tests.md#execution) и [автоматическом](https://docs.yandex-team.ru/devtools/test/automated) запуске тестов выполняется автоматическая проверка правильности импортов. Такая проверка позволяет быстро обнаруживать конфликты между библиотеками или отсутствие каких-то зависимостей в **ya.make** файлах. Проверка правильности импортов — ресурсоёмкая операция, и в настоящий момент выполняется только для исполняемых программ, т.е. модулей использующих один из следующих макросов:
* `PY2_PROGRAM`
* `PY3_PROGRAM`
* `PY2TEST`
* `PY3TEST`
* `PY23_TEST`

Проверяются только модули подключаемые к сборке через макрос `PY_SRCS`.  Проверку можно полностью отключить макросом `NO_CHECK_IMPORTS`:

```yamake
OWNER(g:my-group)

PY3TEST()

PY_SRCS(
    base.py
)

TEST_SRCS(
    test.py
)

SIZE(MEDIUM)

NO_CHECK_IMPORTS() # Отключить проверку импортируемости библиотек из PY_SRCS

NO_CHECK_IMPORTS( # Отключить проверку импортируемости только в указанных модулях
    devtools.pylibrary.*
)

END()
```

Бывает, что в библиотеках есть импорты, которые происходят по какому-то условию:

```python
if sys.platform.startswith("win32"):
    import psutil._psmswindows as _psplatform
```

Если импорт-тест падает в таком месте, можно отключить его следующим образом:

```yamake
NO_CHECK_IMPORTS( # Отключить проверку
    psutil._psmswindows
)
```

## Статический анализ { #lint }

Все файлы на Python используемые, подключаемые в макросах `PY_SRCS` и `TEST_SRCS` в файлах **ya.make**, автоматически проверяются статическим анализатором [Flake8](https://gitlab.com/pycqa/flake8).

{% note info %}

Конфигурационный файл с настройками правил для Flake8 можно посмотреть [здесь](https://a.yandex-team.ru/arc/trunk/arcadia/build/config/tests/flake8/flake8.conf).

{% endnote %}

Для полного отключения таких проверок следует добавить в **ya.make** макрос `NO_LINT()` (допустимо только для директории `contrib`):

```yamake
OWNER(g:my-group)

PY3TEST()

TEST_SRCS(
    test.py
)

SIZE(MEDIUM)

NO_LINT() # Отключить статический анализ

END()

```

Также существует возможность отключить статический анализ отдельных строк в `*.py` файлах при помощи комментария `# noqa`:

```python
# Для этой строчки мы отключаем статический анализатор совсем
from sqlalchemy.orm import Query  # noqa

# Для этой строчки мы игнорируем ошибку E101 во Flake8
from region import Region  # noqa: E101
```

Расшифровку кодов ошибок можно посмотреть на следующих страницах:

* [https://www.flake8rules.com/](https://www.flake8rules.com/)
* [https://pypi.org/project/flake8-commas/](https://pypi.org/project/flake8-commas/)
* [http://www.pydocstyle.org/en/latest/error_codes.html](http://www.pydocstyle.org/en/latest/error_codes.html)
* [https://bandit.readthedocs.io/en/latest/plugins/index.html](https://bandit.readthedocs.io/en/latest/plugins/index.html)



### To be documented

```
PY2TEST/PY3TEST/PY23_TEST... 
TEST_SRCS в PY2_LIBRARY
```

[https://wiki.yandex-team.ru/yatool/test/#python](https://wiki.yandex-team.ru/yatool/test/#python)
