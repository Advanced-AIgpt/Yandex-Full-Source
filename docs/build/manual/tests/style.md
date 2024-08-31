# Проверки кода и корректности данных

## python linters{#flake8}
Все python файлы, используемые в сборке и тестах, подключаемые через `ya.make` в секциях `PY_SRCS` и `TEST_SRCS`, автоматически проверяются `flake8` линтером.

Допустимая длина строки установлена равной 200 символам.

В редких случаях можно игнорировать конкретные строчки целиком, указывая конкретные коды ошибок, добавляя комментарий к нужной строчке вида `# noqa` или `# noqa: E101`.

Внутри `__init__.py` можно подавлять только ошибку `F401` `Module imported but unused` с помощью добавления комментария `# flake8 noqa: F401` в начале файла.

{% note info %}

Только для директории `contrib` допустимо их отключение с помощью макроса `NO_LINT()`.

{% endnote %}

В Аркадии используется единый [конфиг](https://a.yandex-team.ru/arc/trunk/arcadia/build/config/tests/flake8/flake8.conf), через который сразу включаются некоторые плагины.
Изменения в конфиге должны быть согласованы с [python-com@](/devtools/rules/intro#python-committee).

Коды ошибок можно посмотреть на следующих страницах:
 * [flake8rules.com](https://www.flake8rules.com)
 * [pypi.org/project/flake8-commas](https://pypi.org/project/flake8-commas)
 * [pydocstyle.org](http://www.pydocstyle.org/en/latest/error_codes.html#grouping)
 * [bandit.readthedocs.io](https://bandit.readthedocs.io/en/latest/plugins/index.html)

## python imports{#import_test}
Для программ `PY2_PROGRAM`, `PY3_PROGRAM`, `PY2TEST`, `PY3TEST`, `PY23_TEST`, собранных из модулей на питоне, мы добавили проверку внутренних модулей на их импортируемость - `import_test`.
Это позволит обнаруживать на ранних стадиях конфликты между библиотеками, которые подключаются через `PEERDIR`, а также укажет на неперечисленные в `PY_SRCS` файлы (но не `TEST_SRCS`).

Проверять герметичность каждого python модуля достаточно дорого - требуется сборка python и его компоновка с целевым модулем только для одного теста, поэтому проверка импортируемости добавляются только для исполняемых программ, которые будут работать в таком виде в конечном счёте на production.
По этой причине проверяются только модули подключаемые к сборке через `PY_SRCS`, но не через `TESTS_SRCS`.

Проверку можно отключить c помощью макроса `NO_CHECK_IMPORTS`, который принимает список масок модулей, которые не нужно проверять:
```cmake
NO_CHECK_IMPORTS(
    devtools.pylibrary.*
)
```

В программах можно написать `NO_CHECK_IMPORTS()` без параметров, чтобы полностью отключить проверку импортов.

Бывают случаи, когда в общих библиотеках есть импорты, которые происходят по какому-то условию, например в `psutil`:
```python
if sys.platform.startswith("win32"):
    import psutil._psmswindows as _psplatform
```
При этом сама библиотека остается работоспособной, но импорт-тест будет падать, потому что проверяет **все** модули влинкованные в бинарник, а у  `psutil._psmswindows` есть завязки на Windows.
В таком случае стоит написать в ней:
```cmake
NO_CHECK_IMPORTS(
    psutil._psmswindows
)
```

## python style {#black}

Для python3 проектов, можно добавить макрос `STYLE_PYTHON()`, который будет генерировать тест проверяющий соответствие кода в модуле аркадийному style guide.
В качестве линтера использует [black](https://black.readthedocs.io/en/stable/), который применяется в [ya style](https://docs.yandex-team.ru/yatool/commands/style).

{% note info %}

Макрос `STYLE_PYTHON()` можно указывать только для `PY3*` и `PY23*` типов модулей.

{% endnote %}

Для быстрого добавления макроса в проект, можно воспользоваться следующей командой:

```bash
$ cd <project>
$ ya project macro add STYLE_PYTHON --recursive --quiet --after PY3_LIBRARY --after PY23_LIBRARY --after PY3TEST --after PY23TEST --after PY3_PROGRAM
```

Для запуска только `black` тестов внутри проекта используйте команду `ya test --test-type black`.

Если проект состоит как из python2 так и python3 кода, то применение `ya style` может быть затруднено тем, что по умолчанию используется black, который уже не поддерживает python2 код.
Поэтому для части исходных кодов нужно применять `ya style`, для другой `ya style --py2`, что не всегда удобно.
С помощью временно ключа `--fix-style` можно поправить стиль, в модулях, у которых добавлен макрос `STYLE_PYTHON()`.

```
$ cd <project>
$ ya test --test-type black --fix-style
```

Для автоматического применения ya style в PyCharm, см [заметку](https://wiki.yandex-team.ru/balance/fintools/pycharm-ya-style/) от srg91@.

## java style {#java.style}
На все исходные тексты на java, которые подключены через `ya.make` в секции `JAVA_SRCS`, включены автоматические проверки java codestyle.
Сами проверки осуществляются при помощи утилиты [checkstyle](http://checkstyle.sourceforge.net checkstyle) версии [7.6.1](http://checkstyle.sourceforge.net/releasenotes.html#Release_7.6.1).

Мы поддерживаем 2 уровня "строгости" - обычный и строгий (большее количество проверок).
Включить строгий можно добавив макрос `LINT(strict)` в `ya.make` файл соответствующего проекта.

Конфигурационные файлы `checkstyle` для обычных и строгих проверок находятся в директории [resource](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/jstyle-runner/java/resources), табличка с описанием находится [тут](https://wiki.yandex-team.ru/yatool/test/javacodestyle).

## java classpath clashes{#classpath.clash}
Можно опционально проверять наличие дублирующихся классов в classpath при компиляции java проекта.
В проверке участвует не только имя класса, но и хэш файла с его исходным кодом, так как идентичные классы из разных библиотек проблем вызывать не должны.
Для включения этого типа тестов в `ya.make` файл соответствующего проекта, нужно добавить макрос `CHECK_JAVA_DEPS(yes)`.

## sandbox resource {#validate_resource}
В тестах можно использовать Sandbox ресурсы в виде зависимостей.
Такие ресурсы должны иметь `ttl` = `inf`, для того чтобы тесты сохраняли свою работоспособность в прошлом, даже после того как новая версия теста начнёт использовать другую версию ресурса.
Поэтому ко всем suite, использующим sandbox ресурсы, добавляется автоматический тест `validate_resource`, который проверяет `ttl` ресурса.
