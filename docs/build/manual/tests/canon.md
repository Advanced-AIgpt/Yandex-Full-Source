# Канонизация

У тестов есть возможность сообщить о своих результатах в виде данных, которые необходимо сравнить с каноническими.

Чтобы система имела доступ к каноническим данным, их нужно в первый раз канонизировать.

Необходимо отметить, что сравнение с каноническими значениями происходит на стороне `ya`, следовательно, чтобы иметь возможность верифицировать такие тесты, их надо запускать через `ya make`.

Каждый фреймворк для написания тестов предоставляет свою поддержку механизма канонизации, описанную в соответствующих разделах.

## Канонизация в C++

{% note warning %}

На данный момент нативная канонизация в C++ тестах не поддержана.

[DEVTOOLS-1467](https://st.yandex-team.ru/DEVTOOLS-1467)

{% endnote %}

Прямо сейчас, можно канонизировать вывод от тестов или выходные файлы с помощью отдельного ya.make с `EXECTEST()`, который будет зависеть от теста и запускать вручную указанные тесты.
[Документация про EXECTEST](exectest). Также см. [пример](https://a.yandex-team.ru/arc/trunk/arcadia/saas/searchproxy/unistat_signals/ut_canonize/ya.make). 

## Канонизация в Python

Тест сообщает о данных, которые нужно сравнить с каноническими, путем их возврата из тестовой функции командой `return`. На данный момент поддерживаются все простые типы данных, списки, словари и файлы:

```python
def test():
    return [1, 2.0, True]
```

### Канонизация файлов

Для того, чтобы вернуть файл, необходимо воспользоваться [функцией](https://a.yandex-team.ru/arc_vcs/library/python/testing/yatest_common/yatest/common/canonical.py?rev=r8979388#L24)

```python
yatest.common.canonical_file(path, diff_tool=None, local=False, universal_lines=False, diff_file_name=None, diff_tool_timeout=None)
```

* `path`: путь до файла;
* `diff_tool`: путь к программе для сравнения канонических файлов. По умолчанию используется `diff`. [Нестандартные программы для сравнения канонических файлов](#diff_tool);
* `local`: сохранять файл в репозиторий, а не в Sandbox. Мы настоятельно не рекомендуем хранить бинарные канонические файлы в репозитории;
* `universal_lines`: нормализует EOL;
* `diff_file_name`: название для файла с дифом. По умолчанию - `<имя сравниваемого файла>.diff`);
* `diff_tool_timeout`: таймаут на запуск diff tool.

Для канонизации нескольких файлов, необходимо вернуть список или словарь с объектами `canonical_file`.

```python
def test1():
    return [yatest.common.canonical_file(output_path1), yatest.common.canonical_file(output_path2)]
def test2():
    return {
        "path1_description": yatest.common.canonical_file(output_path1),
        "path2_description": yatest.common.canonical_file(output_path2)
    }
```

#### Пример

[Канонизация файла](https://a.yandex-team.ru/arc_vcs/devtools/dummy_arcadia/test/diff_test/test.py)

### Канонизация директорий

Для того, чтобы канонизировать содержимое директории, необходимо воспользоваться [функцией](https://a.yandex-team.ru/arc_vcs/library/python/testing/yatest_common/yatest/common/canonical.py?rev=989d6c250ba32079482bda10552705d8dc306831#L50)
```python
yatest.common.canonical_dir(path, diff_tool=None, diff_file_name=None, diff_tool_timeout=None)
```
* `path`: путь до директории;
* `diff_tool`: путь к программе для сравнения канонических файлов. По умолчанию используется `diff`. [Нестандартные программы для сравнения канонических файлов](#diff_tool);
* `diff_file_name`: название для файла с дифом. По умолчанию - `<имя сравниваемой директории>.diff`);
* `diff_tool_timeout`: таймаут на запуск diff tool.

{% note warning %}

Все директории загружаются в sandbox, их нельзя сохранять локально в `canondata`, в отличие от `canonical_file`.

{% endnote %}

#### Пример

[Канонизация директории](https://a.yandex-team.ru/arc_vcs/devtools/ya/test/tests/canonize_new_format/convert/test_canonical_dir.py)

### Канонизация запуска программы

Для того, чтобы канонизировать `stdout` программы нужно воспользоваться [функцией](https://a.yandex-team.ru/arc_vcs/library/python/testing/yatest_common/yatest/common/canonical.py?rev=989d6c250ba32079482bda10552705d8dc306831#L62)
```python
yatest.common.canonical_execute(binary, args=None, check_exit_code=True, shell=False, timeout=None, cwd=None, env=None, stdin=None, stderr=None, creationflags=0, file_name=None, save_locally=False)
```

* `binary`:абсолютный путь до программы;
* `args`: аргументы программы;
* `check_exit_code`: бросает `ExecutiopnError`, если запуск программы завершается с ошибкой;
* `shell`: использовать `shell`;
* `timeout`: таймаут на выполнении програмы;
* `cwd`: рабочая директория;
* `env`: окружение для запуска команды;
* `stdin`: поток ввода команды;
* `stderr`: поток ошибок команды;
* `creationflags`: [`creationFlags`](https://docs.python.org/3/library/subprocess.html#subprocess.Popen) команды запуска;
* `file_name`: имя output файла. По умолчанию используется имя программы `binary`. Конечное название файла будет `<file_name>.out.txt`
* `save_locally`: сохранять файл в репозиторий, а не в Sandbox. Мы настоятельно не рекомендуем хранить бинарные канонические файлы в репозитории;
* `diff_tool`: путь к программе для сравнения канонических файлов. По умолчанию используется `diff`. [Нестандартные программы для сравнения канонических файлов](#diff_tool);
* `diff_file_name`: название для файла с дифом;
* `diff_tool_timeout`: таймаут на запуск diff tool.

Для канонизации `stdout` запуска python-скриптов можно воспользоваться [функцией](https://a.yandex-team.ru/arc_vcs/library/python/testing/yatest_common/yatest/common/canonical.py?rev=989d6c250ba32079482bda10552705d8dc306831#L107)

```python
yatest.common.canonical_py_execute(script_path, args=None, check_exit_code=True, shell=False, timeout=None, cwd=None, env=None, stdin=None, stderr=None, creationflags=0, file_name=None)
```

Если нужно канонизировать вывод нескольких программ, то результат `canonical_execute` можно сохранять в переменные, а в тесте вернуть словарь:

```python
res1 = yatest.common.canonical_execute(binary1)
res2 = yatest.common.canonical_execute(binary2)
return {"stand_initializer": res1, "prog2": res2}
```

#### Примеры

* [Канонизация stdout программы](https://a.yandex-team.ru/arc_vcs/devtools/ya/test/tests/canonize_new_format/misc/test_2.py#L117)
* [Канонизация stdout python script](https://a.yandex-team.ru/arc_vcs/devtools/ya/test/tests/canonize_new_format/misc/test_2.py#L118)

## Канонизация в Java

Чтобы канонизировать данные, нужно использовать функции из [devtools/jtest](https://a.yandex-team.ru/arc_vcs/devtools/jtest/ya.make).

### Канонизация объектов

Для канонизации объектов нужно использовать [функцию](https://a.yandex-team.ru/arc_vcs/devtools/jtest/src/main/java/ru/yandex/devtools/test/Canonizer.java#L14) `ru.yandex.devtools.test.Canonizer.canonize(Object)`.
Важно помнить, что объект будет сериализован с помощью [`new Gson().toJson(obj)`](https://github.com/google/gson).

{% note warning %}

На каждый тест может быть только один вызов `ru.yandex.devtools.test.Canonizer.canonize(Object)`: если их будет несколько, последний перетрет изменения всех предыдущих.

{% endnote %}

#### Пример

[Канонизация объектов](https://a.yandex-team.ru/arc_vcs/devtools/ya/test/tests/canonize_new_format/java_tests/CanonizeTest.java)

### Канонизация файлов

Для того, чтобы канонизировать файл, нужно использовать [`ru.yandex.devtools.test.CanonicalFile`](https://a.yandex-team.ru/arc_vcs/devtools/jtest/src/main/java/ru/yandex/devtools/test/CanonicalFile.java#L16).

По умолчанию все файлы загружаются в Sandbox. Чтобы сохранить локально, нужно задать параметр `local` в `true`. Мы настоятельно не рекомендуем хранить бинарные канонические файлы в репозитории.

Помимо обычного `diff`, можо использовать кастомный `diff_tool`. Подробнее, как правильно его оформлять, описано [здесь](#diff_tool).

Если в качестве `diff_tool` используется `JAVA_PROGRAM`, то в таком варианте путь к ней необходимо передавать вместе с путем до `java`.

#### Примеры

* [Канонизация файлов](https://a.yandex-team.ru/arc_vcs/devtools/ya/test/tests/canonize_new_format/java_tests/CanonizeTest.java?rev=39f7e844d78e384deb67102e46f40e33c65d70c5#L44)
* [Использование JAVA_PROGRAM как diff tool](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tests/java_diff_tool)

## Канонизация в Go

Для канонизации данных нужно использовать [`library/go/test/canon`](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/test/canon).


### Канонизация объектов

Для канонизации внутриязыковых объектов нужно использовать функцию [`SaveJSON`](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/test/canon/canon.go?rev=r7114367#L80).

#### Пример

[Канонизация внутреязыковых объектов](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/ya/test/tests/canonize_new_format/canonize_struct/lib_test.go)

### Канонизация файлов

Для того, чтобы канонизировать файл, нужно использовать [`SaveFile`](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/test/canon/canon.go?rev=r7114367#L100).

По умолчанию все канонизированные файлы загружаются в `mds/sandbox`.Чтобы сохранять эти файлы локально, нужно в фукнцию `SaveFile` передать аргумент `canon.WithLocal(true)`.

Помимо обычного `diff`, можо использовать кастомный `diff_tool`. Подробнее, как правильно его оформлять, описано [здесь](#diff_tool).

#### Примеры

* [Канонизация файлов](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/ya/test/tests/canonize_new_format/canonize_file_wo_diff_tool/lib_test.go)
* [Канонизация с нестандартным diff tool](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/ya/test/tests/canonize_new_format/canonize_file_with_diff_tool/lib_test.go)

## Diff tool для сравнения канонических файлов и директорий { #diff_tool }

По умолчанию, во всех фреймворках для сравнения канонизированных данных используется `diff`, но для каждого языка есть возможность использовать кастомный `diff tool`.

Для того, чтобы переопределить программу для сравнения канонических файлов, нужно:

1. Добавить в секцию `DEPENDS` теста путь к `ya.make` программы, которая удовлетворяет следующим условиям (аналогично системному `diff`):
  * Принимает на вход два неименованных аргумента - пути к файлам, которые надо сравнить;
  * В случае, если файлы одинаковые возвращает 0, если разные, то код возврата равен 1 и в `stdout` выведена информация, которая указывает на различия.
1. В тестах в соответствующих функциях для канонизации файла или директории передать путь к програме.

{% note info %}

Программа сравнения вызывается только в случае расхождения чек-суммы полученного тестом файла с каноническим: это надо учитывать при отладке diff tool.

{% endnote %}

## Как канонизировать

Для того, чтобы канонизировать результат теста, нужно воспользоваться опцией `-Z, --canonize-tests`:

```shell
  ya make -tF <test name> --canonize-tests [--owner <owner> --token <sandbox token>]
```

Канонический результат будет сохранен рядом с тестом в репозитории в директорию `canondata/<test name>/result.json` или вынесен в ресурс Sandbox, в зависимости от переданных параметров в тесте. Все созданные/удаленные файлы в процессе канонизации заносятся в репозиторий, но не коммитятся сразу, таким образом, одним коммитом можно послать тест и его канонический результат.

{% note warning %}

**Не меняйте руками никакие данные внутри директории** `canondata` - это приведёт к тому, что тест будет работать некорректно, потому что до сверки канонических данных `ya make` проверяет чек-суммы из `canondata/result.json`, и если они расходятся, то только в этом случае строит diff. Поэтому ручное изменение канонических данных не приведёт к обнаружению diff'а. Всегда переканонизируйте результаты с помощью `ya make -AZ`.

{% endnote %}

## Канонизация в Sandbox

Канонизировать результаты тестов можно с помощью sandbox задач [`YA_MAKE` и `YA_MAKE_2`](https://docs.yandex-team.ru/ya-make/usage/sandbox/ya_make).

Для этого нужно:

* в поле `Targets` указать путь до теста;
* выбрать `Run tests`;
* выбрать `Canonize tests`.

Если вы пользуетесь Arc:

* в поле `Svn url for arcadia` нужно указать url в следующем формате: `arcadia-arc:/#<branch name>`, например `arcadia-arc:/#trunk` или `arcadia-arc:/#users/heretic/remove-check`
* выбрать `Use arcadia-api fuse` и `Use arc fuse instead of aapi`

Если вы пользуетесь SVN:

* в поле `Svn url for arcadia` нужно указать url в следующем формате: `arcadia:/#<branch name>`, например `arcadia:arc/arcadia/trunk`

После завершения работы таски, в информации задачи будет написано, как применить канонизированные данные к вашей локальной копии репозитория.

## Дополнительные опции канонизации

* `--owner`: имя владельца ресурса с каноническими данными в Sandbox. По умолчанию, имя текущего пользователя;
* `--user`: имя пользователя для авторизации в Sandbox. По умолчанию, имя текущего пользователя;
* `--token`: токен для авторизации в Sandbox. Необходимо получить [на странице в Sandbox](https://sandbox.yandex-team.ru/oauth/). Если токен не передан, будет произведена попытка авторизации по SSH-ключам (поддерживаются rsa/dsa-ключи), которые ожидаются или в `~/.ssh`, или в SSH-агенте. Публичная часть ключа должна быть загружена на Стафф;
* `--key`: путь к приватной части rsa/dsa-ключа для авторизации в Sandbox. Публичная часть этого ключа должна быть загружена на Стафф;
  
Если протокол загрузки канонических данных в Sandbox не задан, то предпочтение будет отдано http. Протокол можно задать явно:
* `--canonize-via-skynet`: загрузка только через протокол skynet: на канонические данные, которые нужно загрузить в Sandbox, делается `sky share`, потом rbtorrent-ссылка используется для загрузки;
* `--canonize-via-http`: загрузка только через протокол http.

## Просмотр ретроспективы канонического результата теста

Для того, чтобы посмотреть, как менялся канонический результат теста, нужно прогнать конкретный тест в режиме `--canon-diff`
`ya make -t --canon-diff PREV`.
* Можно передать имя теста через параметр `-F(--test-filter)`, для этого можно сначала вывести список тестов в текущей папке ( `arcadia/ya make -t --canon-diff HEAD -L`);
* В качестве аргумента `--canon-diff` можно передать `PREV`, `HEAD`, `<rev1>:<rev2>`. Данным режимом удобно пользоваться, когда результат частично или целиком был загружен в Sandbox, и `svn diff` не очень помогает.
