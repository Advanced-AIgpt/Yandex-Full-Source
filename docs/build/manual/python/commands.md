# Python : вспомогательные команды
## ya py

Собрать IPython-консоль со встроенными библиотеками.

**Как запустить**
```
ya py [OPTIONS] [TARGETS]
```

Команда создаёт программу с влинкованными библиотеками `TARGETS` и входом в интерпретатор IPython и запускает её.
Это позволяет посмотреть как работают различные функции из библиотеки прямо в интерпретаторе. В качестве `TARGETS`
могут выступать один или несколько модулей [`PY2_LIBRARY`](modules.md#py_library), [`PY3_LIBRARY`](modules.md#py3_library), [`PY23_LIBRARY`](modules.md#py23_library), а также один (и только один) модуль типа [`PY2_PROGRAM`](modules.md#py_program) или [`PY3_PROGRAM`](modules.md#py3_program).

{% note warning %}

Поскольку программа собирается с конкретной версией Python, `TARGETS` должны быть совместимы между собой.
Для Python 2 допустимы [`PY2_LIBRARY`](modules.md#py_library), [`PY23_LIBRARY`](modules.md#py23_library) и [`PY2_PROGRAM`](modules.md#py_program).
Для Python 3 допустимы [`PY3_LIBRARY`](modules.md#py_library), [`PY23_LIBRARY`](modules.md#py23_library) и [`PY3_PROGRAM`](modules.md#py3_program).

{% endnote %}

Если в `TARGETS` входит модуль типа [`PY2_PROGRAM`](modules.md#py_program) или [`PY3_PROGRAM`](modules.md#py3_program) запуск команды будет аналогичен запуску исходного приложения, но со входом в интерпретатор. Т.е. будет собрана программа со всеми её зависимостями после чего запустится интерпретатор.

{% note tip %}

`TARGETS` может содержать одну программу и несколько библиотек одновременно. Для импорта в интерпретаторе будут доступны все модули из программ плюс модули из библиотек, перечисленных в `TARGETS`.

{% endnote %}


**Пример:**

```
[=] ya py library/python/guid
Creating temporary PY2_PROGRAM /home/spreis/ws/arcadia/junk/username/_ya_py
Baking shell with:
 library/python/guid
Ok
Python 2.7.16 (default, Dec 28 2020, 11:22:50)
Type "copyright", "credits" or "license" for more information.

IPython 5.9.0 -- An enhanced Interactive Python.
?         -> Introduction and overview of IPython's features.
%quickref -> Quick reference.
help      -> Python's own help system.
object?   -> Details about 'object', use 'object??' for extra details.

In [1]: import library.python.guid as g

In [2]: print g.guid()
1c0fa9ea-ad758955-d7e0d4ff-7c1369c2
```

`ya py` может создавать консоль как для Python 2, так и для Python 3. Версия питона автоматически определяется модулем в `[TARGETS]`.
Для [PY23_LIBRARY](modules.md#py23_library) версию можно выбрать ключами `-2` и `-3` соответственно.


**Пример:**

```
[=] ya py -3 library/python/guid
Creating temporary PY3_PROGRAM /home/spreis/ws/arcadia/junk/spreis/_ya_py
Baking shell with:
 library/python/guid
Ok
Python 3.8.7 (default, Dec 28 2020, 14:14:23)
Type 'copyright', 'credits' or 'license' for more information
IPython 7.19.0 -- An enhanced Interactive Python. Type '?' for help.

In [1]: import library.python.guid as g

In [2]: print(g.guid())
83f8ea54-2e441506-8e048ef6-d79feb36
```

`ya py` поддерживает существенную часть опций команды `ya make` в том числе, например, возможность выкачать недостающие исходные файлы из svn.

**Полезные опции**
   ```
       -r                  Релизная сборка (с оптимизациями, по умолчанию - отладочная)
       --checkout          Подвезти недостающие директории (только для svn)
       -2, --py2           Построить консоль для Python 2
       -3, --py3           Построить консоль для Python 3
       -b, --py-bare       Построить консоль без дополнительных модулей
       -DFLAG              Задать сборочный флаг
   ```


## pip2arc

Утилита для импорта внешнего кода в contrib/python.

```
pip2arc [-h] -a DIR [-s DIR] [--owner OWNER] [--contrib-csv FILE] [--dst DIR] [--rm] [--tests] package [package ...]
```

Импортирует пакет `package` как аркадийный проект.

**Основные опции**
```
  -h, --help                   Показать справку
  -a DIR, --arcadia DIR        Путь до Аркадии                        
  -s DIR, --site-packages DIR  Путь до site-packages (по умолчанию: lib/python*/site-packages)
  --owner OWNER                Назначить владельца (по умолчанию: g:python-contrib)
```

**Дополнительные опции**
```
  --contrib-csv FILE    Путь для contrib.csv (по умолчанию: {arcadia}/devtools/pip2arc/contrib.csv)
  --dst DIR             Директория для результатов (по умолчанию: {arcadia}/contrib/python)
  --rm                  Очистить директорию результатов
  --tests               Импортировать тесты
```

### Порядок работы

Чтобы воспользоваться pip2arc нужно собрать саму утилиту, привезти нужный пакет и затем собственно запустить импорт. 
Далее предполагается,
- Что у вас уже есть Аркадия в ~/arcadia и сборка в ней нормально запускается и работает.
- У вас есть настроенный Python, позволяющий создать виртуальное окружение (доступна команда `virtualenv`).
- Установлен и настроен `pip`.

#### С чего начать

Первым делом соберём `pip2arc`:
```
[=] cd arcadia
[=] ya make -r devtools/pip2arc -I ~/bin
Ok
[=] ls ~/bin
pip2arc                                       -a
[=] ~/bin/pip2arc
usage: pip2arc [-h] -a DIR [-s DIR] [--owner OWNER] [--contrib-csv FILE] [--dst
pip2arc: error: the following arguments are required: -a/--arcadia, package
```

В результате утилита `pip2arc` будет собрана в `~/bin`.


#### Готовим пакет для импорта

Теперь можно создать виртуальное окружение и переключиться на него

```
[=] virtualenv venv
[=] cd venv
[=] . bin/activate
[(venv) =]
```

После этого можно установить нужный пакет в наше окружение
```
[(venv) =] pip install xmltodict
Collecting xmltodict
  Downloading https://files.pythonhosted.org/packages/28/fd/30d5c1d3ac29ce229f6bdc40bbc20b28f716e8b363140c26eff19122d8a5/xmltodict-0.12.0-py2.py3-none-any.whl
Installing collected packages: xmltodict
Successfully installed xmltodict-0.12.0
```

Всё готово к импорту: у нас есть виртуальное окружение с установленным пакетом.

#### Импортируем

Запускаем утилиту `pip2arc`. Как минимум нужно указать пакет для импорта и путь до Аркадии. 
Можно сначала проимпортировать пакет в отдельную директорию (указанную в параметре `--dst`) и потом перенести, а можно сразу в Аркадию как в нашем примере.

{% note tip %}

В зависимости от того, как установлен Python, может потребоваться указать `site-packages` как в примере ниже.

{% endnote %}

{% note info %}

pip2arc позволяет добавить пакет вместе с тестами. К сожалению тесты добавляются как `PY2TEST` (для Python 2) в то время как сам пакет добавляется как [`PY23_LIBRARY`](modules.md#py23_library)

{% endnote %}


```
[(venv) =] bin/pip2arc -a arcadia -s venv/lib/python3.6/site-packages xmltodict
Add xmltodict 0.12.0

+ rsync -L -0 --exclude=INSTALLER --exclude=METADATA --exclude=REQUESTED --exclude=RECORD --exclude=WHEEL venv/lib/python3.6/site-packages/xmltodict-0.12.0.dist-info ws/arcadia/contrib/python/xmltodict --files-from /var/tmp/subcopyvy6n6xmx
+ rsync -L -0 venv/lib/python3.6/site-packages/xmltodict-0.12.0.dist-info ws/arcadia/contrib/python/xmltodict/.dist-info --files-from /var/tmp/subcopyg4taqfw8

[(venv) =] cd arcadia
[(venv) =] arc status
On branch pip2arc_test
Changes not staged for commit:
  (use "arc add/rm <file>..." to update what will be committed)
  (use "arc checkout <file>..." to discard changes in working directory)

     modified:    contrib/python/ya.make
     modified:    devtools/pip2arc/contrib.csv

Untracked files:
  (use "arc add <file>..." to include in what will be committed)

    contrib/python/xmltodict/

no changes added to commit (use "arc add" and/or "arc commit -a")

[(venv) =] ls contrib/python/xmltodict
LICENSE  xmltodict.py  ya.make

[(venv) =]  arc diff contrib/python/ya.make
--- contrib/python/ya.make      (index)
+++ contrib/python/ya.make      (working tree)
@@ -998,6 +998,8 @@ RECURSE(
     XlsxWriter
     xlutils
     xlwt
+    xmltodict
     xxhash
     yandex-pgmigrate
     yappi
```

Таким образом наш пакет добавлен в Аркадию и подключен к автосборке. Осталось только проверить и закоммитить.


#### Проверяем

Для проверки воспользуемся командой [`ya py`](#ya-py). Соберём интерактивную консоль с нашей библиотекой и попробуем её проимпортировать.

```
[=] ya py -3 contrib/python/xmltodict
Creating temporary PY3_PROGRAM /home/spreis/ws/arcadia/junk/spreis/_ya_py
Baking shell with:
 contrib/python/xmltodict
Ok
Python 3.8.7 (default, Dec 28 2020, 14:14:23)
Type 'copyright', 'credits' or 'license' for more information
IPython 7.19.0 -- An enhanced Interactive Python. Type '?' for help.

In [1]: import xmltodict

In [3]: print(xmltodict)
<module 'xmltodict' from 'contrib/python/xmltodict/xmltodict.py'>

In [4]: doc = xmltodict.parse('<a prop="x"><b>1</b><b>2</b></a>')

In [5]: doc['a']['b']
Out[5]: ['1', '2']
```

Теперь можно создать пулл-реквест и закоммимитить изменения в Аркадию.

## library/python/sfx/bin

Программа на Python в Аркадии представляет собой интерпретатор Python и весь необходимый Python-код прекомпилированный и сложенный в ресурсы.
Кроме того, программа обычно содержит исходный Python-код. И его можно извлечь, например для того, чтобы исследовать что попало в программу или чтобы исполнить внешним интерпретатором.

Чтобы это сделать надо:

1. Собрать программу `library/python/sfx/bin` простой командой `ya make -r library/python/sfx/bin` из корня Аркадии.

    ```
    [=] ya make -r library/python/sfx/bin
    Ok
    [=] library/python/sfx/bin/sfx --help
    usage: sfx [-h] [-o OUTPUT] [-s SYMLINK] [--keep_src_path] program

    positional arguments:
    program               binary python program path

    optional arguments:
    -h, --help            show this help message and exit
    -o OUTPUT, --output OUTPUT
                            output directory
    -s SYMLINK, --symlink SYMLINK
                            source root to symlink to
    --keep_src_path

    ```

2. И запустить её, указав в качестве параметров результат сборки [`PY3_PROGRAM`](modules.md#py3_program) или [`PY2_PROGRAM`](modules.md#py_program) и выходную директорию.

    **Например:**

    ```
    [=] library/python/sfx/bin/sfx library/python/sfx/bin/sfx -o ../sfx
    Python 2.7.16 (default, Dec 29 2020, 13:29:55)
    [GCC Clang 11.0.0] on linux2
    Type "help", "copyright", "credits" or "license" for more information.
    (InteractiveConsole)

    >>> >>> >>> >>> 
    [=] ls ../sfx
    library  source_map.json  subprocess32.py  subprocess.py  ya.make
    ```

{% note tip %}

Декомпилируются все ресурсы, лежащие в структуре виртуальной файловой системы Python-программы, поэтому в данном примере мы видим `ya.make`. 
Его сложили в программу явно макросом [`RESOURCE_FILES`](https://a.yandex-team.ru/arc/trunk/arcadia/library/python/sfx/bin/ya.make?rev=r4245482#L12).
В обычных программах этого файла не будет

{% endnote %}

По умолчанию структура в которую декомпилируется программа соответствует структуре пакетов и модулей Python, а не структуре директорий из которых эта программа была собрана.
Это означает, что модули, указанные `TOP_LEVEL` оказываются в корне. В нашем примере это `subprocess32.py` и `subprocess.py`.
Модули, для которых указан `NAMESPACE` будут размещены в соответствии с этим указанием. Такое размещение файлов позволяет запустить код внешним интерпретатором - все импорты
будут правильно работать.

Чтобы воссоздать структру из которой программа была собрана, надо добавить ключ `--keep_src_path`

**Например:**

```
[=] library/python/sfx/bin/sfx library/python/sfx/bin/sfx -o ../sfx2 --keep_src_path
Python 2.7.16 (default, Dec 29 2020, 13:29:55)
[GCC Clang 11.0.0] on linux2
Type "help", "copyright", "credits" or "license" for more information.
(InteractiveConsole)

>>> >>> >>> >>> 
[=] ls ../sfx2
contrib  library  source_map.json  ya.make
[=] ls ../sfx2/contrib/deprecated/python/subprocess32/
__init__.py  subprocess32.py  subprocess.py

```

Как можно увидеть, теперь `subprocess32.py` и `subprocess.py` находятся в `contrib/deprecated/python/subprocess32/` как и в Аркадии из которой они были собраны.

Кроме исходных файлов в корень директории складывается файл `source_map.json` описывающий источники всех файлов в получившейся структуре.

Команда `library/python/sfx/bin/sfx` позволяет получить структуру директорий в которой вместо декомпилированных исходных файлов стоят ссылки на соответствующие файлы в Аркадии.
Это может быть полезно если вы пишите код в Аркадии и, заодно, исполняете его внешним интерпретатором. Чтобы добиться такого используйте ключ `-s` или `--symlink` с указанием пути до Аркадии.

**Например:**

```
[=] library/python/sfx/bin/sfx library/python/sfx/bin/sfx -o ../sfx3 -s `pwd`
Python 2.7.16 (default, Dec 29 2020, 13:29:55)
[GCC Clang 11.0.0] on linux2
Type "help", "copyright", "credits" or "license" for more information.
(InteractiveConsole)

>>> >>> >>> >>> spreis@starshils -la ../sfx3
total 92
drwxrwxr-x 3 spreis dpt_yandex_search_devtech_dep08638  4096 Jan 18 08:00 .
drwxrwxr-x 9 spreis                             112424  4096 Jan 18 08:00 ..
drwxrwxr-x 3 spreis dpt_yandex_search_devtech_dep08638  4096 Jan 18 08:00 library
-rw-rw-r-- 1 spreis dpt_yandex_search_devtech_dep08638   903 Jan 18 08:00 source_map.json
lrwxrwxrwx 1 spreis dpt_yandex_search_devtech_dep08638    67 Jan 18 08:00 subprocess32.py -> /home/spreis/ws/arcadia/contrib/deprecated/python/subprocess32/subprocess32.py
-rw-rw-r-- 1 spreis dpt_yandex_search_devtech_dep08638 71062 Jan 18 08:00 subprocess.py
lrwxrwxrwx 1 spreis dpt_yandex_search_devtech_dep08638    54 Jan 18 08:00 ya.make -> /home/spreis/ws/arcadia/library/python/sfx/bin/ya.make
```


## Отладка в gdb и pdb

Поскольку программы на Python - это более-менее обычные программы, их можно отлаживать с помощью `ya tool gdb`.

Для работы python pretty printers в gdb нужно отключить strip при линковке, вызывая `ya make` с опцией `-DNO_STRIP=yes`.

Если запускать `gdb` из корня чекаута, `py-bt` будет печатать не только имена исходных файлов с номерами строк, но и исходный код.

Систему сборки можно попросить запустить Python-тесты под отладчиком командами

```
ya make -ttt --gdb
```
```
ya make -ttt --pdb
```

Эти команды соберут тесты и заставят на старте теста провалиться в соответствующий отладчик.

{% note tip %}

`gdb` по умолчанию не хранит историю от предыдущих запусков. Можно это изменить, написав в `~/.gdbinit` `set history save`. 

Более расширенный вариант настроек:
```
set history save
set history size 100000
set print pretty
set print symbol-filename
set disassembly-flavor intel
set max-value-size 131072
set python print-stack full
```

{% endnote %}


Дополнительные команды отладчика описаны в [документации Python](https://docs.python.org/devguide/gdb.html). 

См. также [Введение в gdb](https://abv.at.yandex-team.ru/583)


## Отладка в IDE PyCharm

С отладкой в IDE пока не всё гладко, но мы над этим работаем. Пока есть только

- [Рецепт от @i-dyachkov](https://i-dyachkov.at.yandex-team.ru/1)
- [Генератор helper-скрипта от @sulim](https://sulim.at.yandex-team.ru/187)
- [Бета ya ide pycharm от @nogert](https://clubs.at.yandex-team.ru/python/3392)

## Виртуальное окружение Python

Команда ```ya ide venv``` позволяет создавать виртуальное окружение Python.

Пример:
```bash
cd ~/arcadia
ya ide venv --venv-root=$HOME/venv --venv-with-pip -r my/python/projects
$HOME/venv/bin/pip install numpy
$HOME/venv/bin/python -c 'import numpy; print("OK")'
```
Полученный интерпретатор `$HOME/venv/bin/python` можно использовать в PyCharm в качестве "Python Interpreter" для отладки `my/python/projects`.

Подробности см. в [документации](../../usage/ya_ide/venv.md).
