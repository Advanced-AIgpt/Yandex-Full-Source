# ya project

Создать или изменить файлы проекта в Аркадии.

`ya project <subcommand>`

## create

Создать новый типовой проект.

`ya project create <project-type> [dest_path] [options]`

Команда может создавать и модифицировать как файлы описания сборки `ya.make`, так и любые другие файлы включая исходный код и/или документацию.

**Пример:**
```
cd arcadia
mkdir <your_project_dir>
cd <your_project_dir>
ya project create <project-type> [options]
```

**или**
```
cd arcadia
mkdir <rel_project_path>
ya project create <project-type> <rel_project_path> [options]
```

`ya project create` поддерживает небольшой набор встроенных типов проектов и расширяемую генерацию проектов по [шаблонам](#templates).
Встроенные типы проектов описаны ниже, полный список доступных типов проектов (включая шаблонные) можно получить командой `ya project create --help`.

{% note warning %}

Команду `ya project create` надо обязательно вызывать из папки в Аркадии. Запуск извне работать не будет даже если в качестве целевой папки будет указан путь в Аркадию.

{% endnote %}

{% note alert %}

Команду `ya project create` крайне желательно запускать в пустой директории. При правильной работе она не станет перезаписывать существующие файлы и просто сломается, но никто не застрахован от ошибок в коде.

{% endnote %}

{% note tip %}

На данный момент встроенные хендлеры не создают директории для проекта при их отсутствии, а шаблонные создают. Со временем мы устраним это различие.

{% endnote %}


### Встроенные типы проектов

Можно создать проекты следующих встроенных типов:

Имя | Описание
:--- | :---
library | Простая C++ библиотека
program | Простая C++ программа
unittest | Тест для C++ на фреймворке unittest. С параметром `--for` создаёт тесты для указанной библиотеки (с исходным код тестов в ней)
mkdocs_theme | Python-библиотека темы mkdocs. Требует указания параметра `--name` - имя шаблона и наличия `__init__.py` в текущей директории.
recurse | Создать `ya.make` для сборки вложенных целей с помощью `RECURSE()`.

Большинство встроенных проектов поддерживают следующие опции:

```
-h, --help        справка по опциям для конкретного проекта
--set-owner       установить владельцем пользователя или группу
--rewrite         перезаписать ya.make
-r, --recursive   создать проекты рекурсивно по дереву директорий
```

**Пример:**

```
[arcadia] mkdir project
[arcadia] cd project

[arcadia/project] mkdir lib
[arcadia/project] cd lib

[arcadia/project/lib] ya project create library --set-owner g:group

[arcadia/project/lib] ls
ya.make

[arcadia/project/lib] cat ya.make
LIBRARY()

OWNER(g:group)

SRCS()

END()

[arcadia/project/lib] cd ..
[arcadia/project] ya project create recurse

[arcadia/project] ls
lib  ya.make

[arcadia/project] cat ya.make
RECURSE(
    lib
)

```

### Шаблоны проектов  { #templates }

Кроме описанный выше встроенных типов проектов, поведение `ya project create` может быть расширено за счёт шаблонов.
Шаблоны проектов храняться в репозитории и зачитываются непосредственно в момент запуска команды. Полный список доступных
типов проектов доступен по команде `ya project create --help`. Список включает как встроенные, так и шаблонные типы.

На момент написания этой документации были доступны следующие шаблонные проекты:

Имя | Описание
:--- | :---
docs | Проект документации
project_template | Шаблон для [создания шаблона проекта](#authoring)
py_library | Пустая Python 3 библиотека с тестами
py_program | Пустая Python 3 программа с тестами
py_quick | Программа на Python 3 из всех .py-файлов в директории

Генерация проекта по шаблону не слишком отличается от генерации встроенных проектов, есть лишь небольшое отличие в работе с опциями. 
Количество общих опций сведено до минимума, из общих опций поддержана только `--set-owner`. Однако, шаблоны могут поддерживать свои опции.
При указании целевой директории их необходимо писать строго после неё. Если опция шаблона совпадает со встроенной, то передать в шаблон опцию можно после `--`.

  **Пример**
  ```
  [arcadia/project] ya project create docs --help
  Create docs project

  Usage:
    ya project create docs [OPTION]... [TAIL_ARGS]...

  Options:
    Ya operation options
      -h, --help          Print help
    Bullet-proof options
      --set-owner=SET_OWNER
                          Set owner of project(default owner is current user)
      --rewrite           Rewrite existing ya.make
  ```
  ```
  [arcadia/project] ya project create docs -- --help 
  usage: Docs project generator [-h] [--name NAME]

  optional arguments:
    -h, --help   show this help message and exit
    --name NAME  Docs project name
  ```

### Добавление шаблона своего проекта  { #authoring }

Шаблон проекта — это, по сути, две опциональных функции на Python 2 и набор опять же опциональных [jinja](https://ru.wikipedia.org/wiki/Jinja)-шаблонов.
Шаблон должен быть размещён в поддиректории в [`devtools/ya/handlers/project/templates`](https://a.yandex-team.ru/arc_vcs/devtools/ya/handlers/project/templates)
и зарегистрирован в [`devtools/ya/handlers/project/templates/templates.yaml`](https://a.yandex-team.ru/arc_vcs/devtools/ya/handlers/project/templates/templates.yaml).

{% note info %}

Несмотря на размещение внутри дерева кода утилиты `ya` код шаблонов не становится кодом `ya`. Этот код зачитывается непосредственно в
момент запуска `ya project create` и интепретируется встроенным в `ya` интепретатором (отсюда требование на Python 2).

{% endnote %}

Создать заготовку для шаблона проекта можно командой `ya project create project_template`, которая сама является примером шаблонной генерации.
Выглядит это примерно так:

```
cd arcadia/devtools/ya/handlers/project/templates
mkdir my_project
ya project create project_template
```

Получится следующая структура директорий:
```
devtools/ya/handlers/project/templates/my_project
├── template
│   └── place-your-files-here
├── README.md
└── template.py
```

`template.py` будет содержать заготовки для двух функций:
```python
def get_params(context):
    """
    Calculate all template parameters here and return them as dictionary.
    """
    env = {}
    return env

def postprocess(context, env):
    """
    Perform any post-processing here. This is called after templates are applied.
    """
    pass
```

#### Готовим параметры

В функции `get_params` нужно получить данные, необходимые для создания шаблона проекта и сформировать словарик `env`, который будет использоваться для подстановки в [jinja](https://ru.wikipedia.org/wiki/Jinja)-шаблоны и передан в `prostprocess`.
Параметры можно получить из переданного контекста (параметр `context`), разбором опций, переданных в контексте или интерактивным запросом у пользователя.

Параметр `context` — это `namedtyple` типа `Context` из [`template_tools.common`](https://a.yandex-team.ru/arc_vcs/devtools/ya/handlers/project/template_tools/common.py).
В нём доступно 5 свойств:
* **`path`** - относительный путь от корня репозитория до создаваемого проекта
* **`root`** - абсолютный путь корня репозитория
* **`owner`** - значение, переданное параметром `--set-owner`
* **`args`** - список неразобранных аргументов, переданных в конце вызова `ya project create <project_name> [path] [args]`
* **`backup`** - объект из [`template_tools.common`](https://a.yandex-team.ru/arc_vcs/devtools/ya/handlers/project/template_tools/common.py),
  позволяющий сохранять оригинальные файлы перед их редактированием. Файлы будут восстановленый в случае исключений во время геренерации проекта,
  а также будут сохранены в `~/.ya/tmp` на случай если что-то пойдёт не так (сохраняются файлы от последних 5 сейссий).

{% note tip %}

Для исполнения кода в шаблонах используется интепретатор влинкованный в утилиту `ya`. Как результат в них доступны все модули доступные в коде `ya`.
Это и специальные утилиты для использования в шаблонах [`template_tools.common`](https://a.yandex-team.ru/arc_vcs/devtools/ya/handlers/project/template_tools/common.py) и
общие библиотеки, например, `pathlib2` и библиотеки самой утилиты `ya`, например [`yalibrary.makelists`](https://a.yandex-team.ru/arc_vcs/devtools/ya/yalibrary/makelists) для работы с файлами `ya.make`.

{% endnote %}

Код для `get_params` может выглядеть, например так:

```python
from __future__ import absolute_import
from __future__ import print_function
from pathlib2 import Path
from template_tools.common import get_current_user

def get_params(context):
    """
    Calculate all template parameters here and return them as dictionary
    """
    print("Generating sample project")

    path_in_arcadia = Path(context.path)
    env = {
        "user": context.owner or get_current_user(),
        "project_name": str(path_in_arcadia.parts[-1]),
    }
    return env
```

Для аккуратного выхода из генерации во время подготовки параметров вместо `env` можно вернуть объект класса 
[`template_tools.common.ExitSetup`](https://a.yandex-team.ru/arc_vcs/devtools/ya/handlers/project/template_tools/common.py).

#### Пишем шаблонны

В поддиректории `template` нужно разместить [jinja](https://ru.wikipedia.org/wiki/Jinja)-шаблоны генерируемых файлов. В них можно использовать подстановку из `env`.
По шаблонам будут генерироваться одноимённые файлы в целевой директории. Обычно среди шаблонов будет один для `ya.make` и, возможно, ещё несколько для исходного кода.

Например, мы можем сделать для нашего проекта такие шаблоны:

**ya.make**
```yamake
OWNER({{ user }})

PROGRAM({{project_name}})

SRCS(
    main.cpp
)

END()
```

**main.cpp**
```cpp
#include <util/stream/output.h>

int main() {
    Cout << "Hello from {{user}}!" << Endl; 
}
```

{% note tip %}

Не забудьте стереть файл `place-your-files-here`

{% endnote %}

{% note tip %}

Если проект не использует шаблоны, а, например, генерирует код в `postprocess` сам, то в директорию `template` надо положить файл `.empty.template`.
Так сделано чтобы было понятно, что шаблонов именно не предполагается, их не забыли положить.

{% endnote %}

Если у вашего типа проекта есть не только шаблон создания, но и [шаблон обновления](#up_templates), то вы можете добавить вот такой шаблон:

**.ya_project.default**
```
name: {{__PROJECT_TYPE__}}
```

Это сделает ваш тип проекта умолчательным в этой директории и для его обновляния можно будет вызывать `ya project update` без явного указания типа проекта.

#### Финальные штрихи

После генерации будет вызвана функция `postprocess`. В ней можно внести финальные штрихи в сегенрированный проект или просто вывести что-то приятное.
В частности, с помощью вызова `add_recurse(context)` из [`template_tools.common`](https://a.yandex-team.ru/arc_vcs/devtools/ya/handlers/project/template_tools/common.py)
можно добавить наш проект в родительской директории.

```python
def postprocess(context, env):
    """
    Perform any post-processing here. This is called after templates are applied.
    """
    from template_tools.common import add_recurse

    add_recurse(context)

    print("You are get to go. Build your project and have fun:)")

    pass
```

{% note tip %}

В этой же функции можно разместить вообще всю генерацию если не хочется использовать шаблоны.

{% endnote %}

#### Подключаем и тестируем

Чтобы наш шаблон стал доступен в `ya project create` нужно зарегистрировать его в файле [`devtools/ya/handlers/project/templates/templates.yaml`](https://a.yandex-team.ru/arc_vcs/devtools/ya/handlers/project/templates/templates.yaml).

Надо добавить в этот файл примерно следующее:

```yaml
- name: my_project                   # Имя типа проекта, которое надо указывать в команде 
  description: Create test project   # Описание для ya project create --help
  create:                            # Шаблон для `ya project create` (есть ещё update)
    path: my_project                 # Относительный путь до директории с шаблоном
```

Теперь можно проверить наш шаблон. Он будет взят во время запуска команды `ya project create`, ничего пересобирать не требуется.

```
[arcadia] ya project create --help
Create project

Usage: ya project create <subcommand>

Available subcommands:
  docs                  Create docs project
  library               Create simple library project
  mkdocs_theme          Create simple mkdocs theme library 
  my_project            Create test project                <<<< Это наш шаблон
  program               Create simple program project
  project_template      Create project template
  py_library            Create python 3 library with tests
  py_program            Create python 3 program with tests
  py_quick              Quick create python 3 program from existing sources without tests
  recurse               Create simple recurse project
  unittest              Create simple unittest project
```

Попробуем его в деле, чтобы проверить все механики создадим ya.make в родительской директории:

```
[arcadia] cat junk/user/tst/ya.make
OWNER(user)
```

Запускаем:

```
[arcadia] ya project create my_project junk/user/tst/myprj 
Generating sample project
You are get to go. Build your project and have fun:)
```

Собираем через родительский ya.make и запускаем:
```
[arcadia] ya make junk/user/tst
[arcadia] junk/user/tst/myprj/myprj
Hello from user!
```

**Структура директорий:**
```
junk/user/tst
├── myprj
│   ├── main.cpp
│   ├── myprj
│   └── ya.make
└── ya.make
```

**Файлы:**

junk/user/tst/ya.make
```yamake
OWNER(user)

RECURSE(
    myprj
)
```

junk/user/tst/myprj/ya.make
```yamake
OWNER(user)

PROGRAM(myprj)

SRCS(
    main.cpp
)

END()
```

junk/user/tst/myprj/main.cpp
```cpp
#include <util/stream/output.h>

int main() {
    Cout << "Hello from user!" << Endl;
}
```

## update

Обновить типовой проект.

`ya project update [project-type] [dest_path] [options]`

Команда может создавать и модифицировать как файлы описания сборки `ya.make`,
так и любые другие файлы включая исходный код и/или документацию. В отличие от `ya project create` эта команда 
предназначена для догенерации или актуализации существующего проекта.

`ya project update` поддерживает небольшой набор встроенных типов проектов и расширяемое обновление проектов по [шаблонам](#up_templates).
Встроенные типы проектов описаны ниже, полный список доступных типов проектов (включая шаблонные) можно получить командой `ya project update --help`.

Некоторые типы проектов могут быть использованы как в `ya project create`, так и в `ya project update`, другие могут работать только в одной из этих команд.

{% note tip %}

`ya project update` без указания типа попробует найти файл с именем `.ya_project.default`. Это файл в формате yaml в котором команда будет искать ключ `name`. Значение этого ключа и будет использовано как тип проекта.
Такой файл может, например, создавать команда `ya project create`.

{% endnote %}

{% note warning %}

Команду `ya project update` надо обязательно вызывать из папки в Аркадии. Запуск извне работать не будет даже если в качестве целевой папки будет указан путь в Аркадию.

{% endnote %}

### Встроенные типы проектов

Доступно обновление для следующих встроенных типов проектов:

Имя | Описание
:--- | :---
recurce | Дописать в ya.make недостающие `RECURSE` на дочерние проекты
resources | Обновить информацию об [автообновляемых ресурсах](../manual/common/data#manual)

Встроенные проектов поддерживают следующие опции:

```
-h, --help        справка по опциям для конкретного проекта
-r, --recursive   обновить проекты рекурсивно по дереву директорий
--dry-run         для `resources` не делать обновление, а лишь показать, что будет обновлено
```

**Пример:**

```
junk/user/libX
├── bin
│   ├── __main__.py
│   └── ya.make
├── dummy
│   └── file.X
├── lib
│   ├── tests
│   │   ├── test.py
│   │   └── ya.make
│   ├── app.py
│   └── ya.make
└── ya.make
```

Допустим не хватает `RECURSE` в двух файлах.

junk/user/libX/ya.make

```yamake
OWNER(user)

RECURSE(
    bin
)
```

junk/user/libX/lib/ya.make
```
OWNER(user)

PY3_LIBRARY()

PY_SRCS(app.py)

END()
```

Запускаем

```
[arcadia] ya project update recurse junk/user/libX  --recursive

Warn: No suitable directories in junk/user/libX/dummy
Info: junk/user/libX/lib/ya.make, RECURSE was updated
Info: junk/user/libX/ya.make, RECURSE was updated
```

Проверяем:

junk/user/libX/ya.make (заметим, что `RECURSE` на `dummy` не нужен, там нет `ya.make`)
```yamake
OWNER(user)

RECURSE(
    bin
    lib
)
```

junk/user/libX/lib/ya.make
```
OWNER(user)

PY3_LIBRARY()

PY_SRCS(app.py)

END()

RECURSE(
    tests
)
```

### Обновление по шаблону  { #up_templates }

Кроме описанный выше встроенных типов проектов, поведение `ya project update` может быть расширено за счёт шаблонов.
Шаблоны проектов храняться в репозитории и зачитываются непосредственно в момент запуска команды. Полный список доступных
типов проектов доступен по команде `ya project update --help`. Список включает как встроенные, так и шаблонные типы.

На момент написания этой документации для обновления были доступны следующие шаблонные проекты:

Имя | Описание
:--- | :---
py_library | Добавить отсутствующие .py-файлы в `PY_SCRS` проекта
py_program | Добавить отсутствующие .py-файлы в `PY_SCRS` проекта
py_quick | Добавить отсутствующие .py-файлы в `PY_SCRS` проекта

Все они используют один и тот же шаблон, просто добавляющий отсутствующие файлы.

Обновление проекта по шаблону не слишком отличается от генерации встроенных проектов, есть лишь небольшое отличие в работе с опциями. 
Количество общих опций сведено до минимума. Однако, шаблоны могут поддерживать свои опции. При указании целевой директории их необходимо писать строго после неё. Если опция шаблона совпадает со встроенной, то передать в шаблон опцию можно после `--`.

Например, обновление проектов для Python поддерживает следующие опции:

```
[arcadia] project update py_quick -- --help
usage: Python project updater [-h] [--recursive]

optional arguments:
  -h, --help   Показать справку
  --recursive  Добавить файлы из поддиректорий тоже
```

Заметим, что `--recursive` здесь и во встроенных проектах имеет разный смысл. Для обновления по шаблону рекурсивная
работа не поддерживается и в данном случае это опция самого шаблона, говорящая, что `.py`-файлы надо собрать рекурсивно и
добавить их все в обновляемый проект.

### Добавление шаблона для обновления

Добавление шаблона для обновления практически не отличается от [добавления шаблона для создания проекта](#authoring)
Есть лишь следующие особенности:

* При созданнии файла по шаблону если целевой файл уже существует, то `ya project create` закончится с ошибкой.
  Обновление `ya project update` файл перезапишет, оригинальный файл будет сохранён и в случае исключения воостановлен.
  Кроме того, он останется в `.ya/tmp` на случай если что-то пойдён не так (хранятся данные от последних 5 запусков).
  При модификации файлов из кода шаблона рекомендуется также использовать объект `backup` передаваемый в контексте.

* Для регистрации шаблона обновления путь надо указывать в секции `update`
  ```yaml
  - name: my_project                   # Имя типа проекта, которое надо указывать в команде 
    description: Create test project   # Описание для ya project create --help
    create:                            # Шаблон для `ya project create`
      path: my_project_up              # Относительный путь до директории с шаблоном создания
    update:                            # <-- Шаблон для `ya project update`
      path: my_project_up              # <-- Относительный путь до директории с шаблоном обновления
  ```
  Секция `create` не является обязательной, могут быть проекты только для обновления.

{% cut "Пример шаблона для обновления py_update" %}.

**devtools/ya/handlers/project/templates/py_update/template.py**

```python
from __future__ import absolute_import
from __future__ import print_function

import os
import argparse

import yalibrary.makelists as makelists
from template_tools.common import get_current_user, glob, touch

def get_params(context):
    """
    Calculate all template parameters here and return them as dictionary
    """
    env = {
        'recursive': False
    }
    if len(context.args) > 0:
        parser = argparse.ArgumentParser('Python project updater')
        parser.add_argument('--recursive', action='store_true', help='Add python files recursively')
        parsed_args = parser.parse_args(context.args)
        env = {
            'recursive': parsed_args.recursive,
        }
    return env

def postprocess(context, env):
    """
    Perform any post-processing here. This is called after templates are applied.
    """
    abs_path = os.path.join(context.root, context.path)
    ya_make = os.path.join(abs_path, 'ya.make')

    if not os.path.isfile(ya_make):
        print('Error: ya.make not found, exiting...')
        return

    makelist = makelists.from_file(ya_make)
    macros = makelist.find_siblings('PY_SRCS')
    py_srcs_macro = None
    if not macros:
        py_srcs_macro = makelists.macro_definitions.Macro.create_node('PY_SRCS')
        makelist.children.append(py_srcs_macro)
    else:
        py_srcs_macro = macros[-1]

    py_files = glob('*.py', recursive=env['recursive'], path=abs_path)

    known = set(map(lambda item: item.name, py_srcs_macro.get_values()))
    added = 0
    for py in py_files:
        if py not in known:
            py_srcs_macro.add_value(py)
            added += 1

    if added:
        context.backup.add(ya_make)
        makelist.write(ya_make)
        print('Done, added {} files to PY_SRCS'.format(added))

```

{% endcut %}

- Полный код [доступен здесь](https://a.yandex-team.ru/arc_vcs/devtools/ya/handlers/project/templates/py_update)
- Шаблон модифицирует `ya.make` в своём коде, поэтому шаблонных файлов в нём нет.
- Шаблон поддерживает свои опции.
- Шаблон использует библиотеку [`yalibrary.makelists`](https://a.yandex-team.ru/arc_vcs/devtools/ya/yalibrary/makelists) для модификации файлов `ya.make`.

## fix-peerdirs

Добавить недостающие и удалить ненужные `PEERDIR` в проекте. Нужность определяется анализом зависимостей исходных файлов проекта.

```
ya project fix-peerdirs [OPTION]... [TARGET]...
```

{% note alert %}

Реализация команды давно не обновлялась и потому результат может быть неточным. Желательно проверять глазами и тестами до коммита.

{% endnote %}

**Опции**
```
    -h, --help          Справка
    -a                  Только добавить недостающие PEERDIR
    -d                  Только удалить лишние PEERDIR
    -v                  Подробная диагностика
    -l                  Только диагностика
    --sort              Отсортировать PEERDIR
    -c, --cycle         Обнаруживать циклические зависимости
```

## macro

Добавить или удалить макрос в ya.make

```
ya project macro add <macro_string> [OPTIONS]

```

```
ya project macro remove <macro_name>
```

**Опции (только для add):**
```
    --after=SET_AFTER   Добавить после макроса 
    --append            Добавить аргументы к существующему макросу
```

**Пример:**

```
[arcadia/project/lib] cat ya.make
LIBRARY()

OWNER(g:group)

SRCS()

END()

[arcadia/project/lib$] ya project macro add 'SRCS(a.cpp)' --append

[arcadia/project/lib] cat ya.make
LIBRARY()

OWNER(g:group)

SRCS(
    a.cpp
)

END()
```

## owner

Работа с владельцами в ya.make.

```
ya project owner [<subcommand>] [OPTION]... [TARGET]...
```

**Доступные команды:**

Подкоманда | Описание
:--- | :---
<без команды> | Показать владельцев для проекта
`add <owners>` | Добавить владельцев
`check_logins` | Проверить актуальность владельцев. `--is-maternity` и `--is-dismissed` на отпуск по уходу за ребёнком и уволенность соотвественно
`optimize` | Оптимизировать список владельцев: удалить владельцев-пользовалетей если они принадлежат владельцу-группе
`remove <owners>` | Удалить владельцев из ya.make. `--default-owner=<owner>` позволяет проставить владельца, если после удаления владельцев не осталось.
`replace <old^new>` | Заменить в списке владельцев старного на нового.
`set <owners>` | Сделать владельцами `owners`. Все прежние владельцы будут удалены.

**Пример:**

```
[arcadia/project/lib] cat ya.make
LIBRARY()

OWNER(g:group)

SRCS(
    a.cpp
)
END()

[arcadia/project/lib] ya project owners set user

[arcadia/project/lib] cat ya.make
LIBRARY()

OWNER(user)

SRCS(
    a.cpp
)
END()

[arcadia/project/lib] ya project owners replace user^g:users

[arcadia/project/lib] cat ya.make
LIBRARY()

OWNER(g:users)

SRCS(
    a.cpp
)
END()
```
