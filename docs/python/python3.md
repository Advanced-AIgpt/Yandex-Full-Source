
Python 2 [объявлен deprecated](https://clubs.at.yandex-team.ru/arcadia/23044), в связи с этим весь новый код в Аркадии в идеале должен быть
PY3 или PY23.

## Написание PY23 кода.

### PY3 vs PY23

Первая рекомендация - если есть выбор писать py3 код или py23 - всегда лучше выбирать первое.

Почему: py23 требует больше усилий на написание, использование backport библиотек и библиотек обеспечения совместимости делают код сложнее.

Как определить возможность написания сразу py3 кода:
  - ваш код никто не использует, или использует только py3 код;
  - у вас нет py2 only зависимостей, которые вы не можете превратить в py3/py23;

### Тесты

Вне зависимости от того, пишете вы py2 или py23 (или py3) код - нужно писать тесты. Это сильно упростит дальнейшую миграцию py2 -> py23 и py23 -> py3 соответственно.

### Как писать py23 код

[Официальная рекомендация](https://docs.python.org/3/howto/pyporting.html)

[Если нужно перевести на третий питон, сохраняя обратную совместимость лишь на время перехода](http://python-future.org/compatible_idioms.html)

#### Коротко про библиотеки

Стандарт дефакто - [six](https://a.yandex-team.ru/arc/trunk/arcadia/contrib/python/six).

Для использования некоторых фич из библиотеки py3 в py2 используйте backports (пример подключения backports см. в **Как написать ya.make для py23** ниже).

  * [enum34](https://a.yandex-team.ru/arc/trunk/arcadia/contrib/python/enum34)
  * [functools32](https://a.yandex-team.ru/arc/trunk/arcadia/contrib/deprecated/python/functools32)
  * [concurrent futures](https://a.yandex-team.ru/arc/trunk/arcadia/contrib/python/futures)
  * [subprocess32](https://a.yandex-team.ru/arc/trunk/arcadia/contrib/deprecated/python/subprocess32)
    - subprocess32 уже добавляется в сборку [по умолчанию во все python2 программы](https://a.yandex-team.ru/arc/trunk/arcadia/build/ymake.core.conf?rev=7596761#L3063) собираемые в Аркадии, [подменяя builtin модуль subprocess из python2](https://a.yandex-team.ru/arc/trunk/arcadia/contrib/deprecated/python/subprocess32/ya.make?rev=5636793#L27) для всех платформ, кроме windows.

#### Шапка

Если вы пишете PY23 код - полезно в стандартный шаблон для нового python файла в вашем любимом IDE прописать такую шапку:
```python
from __future__ import unicode_literals, absolute_import, division, print_function
```

Правда теперь нужно учесть то, что все строки будут считаться по умолчанию уникодными. Деление будет работать как в 3-м питоне. А print теперь функция. 

### Как написать ya.make для py23

Очень просто. Вместо PY2_LIBRARY используем PY23_LIBRARY. В случае, если есть py2-only / py3-only модули и/или зависимости - используем макром IF (PYTHON2) / IF (PYTHON3) соответственно.
Написать PY23_PROGRAM нельзя - ибо в исполняемый файл можно собраться только с одним интерпретатором.
Вместо макроса PY2TEST используем PY23_TEST [анонс](https://clubs.at.yandex-team.ru/arcadia/22614).

**Пример:**

Приложение:
```yamake
# /youproject/application/ya.make
PY23_LIBRARY()

OWNER(
	zeliboba
)

PEERDIR(
	contrib/python/requests
)

PY_SRCS(
	foo.py
)

IF(PYTHON2)
    PEERDIR(
        contrib/python/futures
        contrib/python/enum34
    )
    PY_SRCS(
        py2_only_baz.py
    )
ENDIF()


IF(PYTHON3)
    PEERDIR(
        contrib/python/aiohttp
    )
    PY_SRCS(
        py3_aiohttp_handle.py
    )
ENDIF()

END()

RECURSE_FOR_TESTS(
    tests
)

```

И тесты:
```yamake
# /youproject/application/tests/ya.make
PY23_TEST()

OWNER(
    zeliboba
)

PEERDIR(
    youproject/application
)

TEST_SRCS(
    test_foo.py
)

IF(PYTHON3)
    TEST_SRCS(
        test_py3_aiohttp_handle.py
    )
ENDIF()

END()
```

### Доработки после автоматических тулов

  * тип str: иногда в него пишут bytes, пользуясь неявным преобразованием. Не решается ничем, кроме тестов, особенно страшно взрывается в случае передачи по сети чего-нибудь, потому что может взорваться вообще в другом потоке, который пытается сделать str.decode. Очень полезно в точках взаимодействия с чем-либо обложиться функциями six.ensure_binary или six.ensure_text.
  * builtin-функции, которые возвращают вместо списка iterable. Немного решается автоматическими инструментами по переводу (там нужно вручную врубать проверку idioms), а далее ручным поиском и заменой всего и вся на comprehensions - они работают одинаково в обоих версиях python.
  * Сравнение с другими типами. __cmp__ - меньшее из зол, потому что оно решается через total_ordering и два метода (сложно, но надежно). Проблема в том, что в python2 валидно следующее выражение: `['a', None, 3].sort()` - и оно даёт устойчивый результат. Решается либо перелопачиванием кода, либо добавлением key=str, но порядок может поменяться.
  * Изменение работы функции hash(). В третьем питоне добавился random seed, и функция перестала давать устойчивый результат.
  * pickle. Не стоит использовать pickle. Примерно никогда. Но если вы его таки использовали, то ... 

### ... если у вас все таки Pickle

(на основе опыта кого:inkoit)

  - в py3 декодируем c "encoding": "latin1"
  - кодируем всегда с версией протокола 2

```python
# See: https://stackoverflow.com/a/28218598/10769689
# WARNING: non-ASCII symbols in pickled resources won't be decoded correctly on
# ANY (2->3 or 3->2) python version change
PICKLE_OPTIONS = {} if six.PY2 else {"encoding": "latin1"}
PICKLE_PROTOCOL_VERSION = 2


def load(file_obj):
    # type: (BinaryIO) -> Any
    return pickle.load(file_obj, **PICKLE_OPTIONS)
    
    
def loads(any_str_obj):
    # type: (AnyStr) -> Any
    bytes_obj = six.ensure_binary(any_str_obj)
    return pickle.loads(bytes_obj, **PICKLE_OPTIONS)
    
    
def loads_base64(b64_any_str):
    # type: (AnyStr) -> Any
    b64_bytes = six.ensure_binary(b64_any_str, encoding="ascii")
    return pickle.loads(base64.b64decode(b64_bytes), **PICKLE_OPTIONS)
    
    
def dump(obj, file_obj):
    # type: (Any, BinaryIO) -> NoReturn
    return pickle.dump(obj, file_obj, protocol=PICKLE_PROTOCOL_VERSION)
    
    
def dumps(obj):
    # type: (Any) -> bytes
    return pickle.dumps(obj, protocol=PICKLE_PROTOCOL_VERSION)
    
    
def dumps_base64(obj):
    # type: (Any) -> str
    return base64.b64encode(dumps(obj)).decode("ascii")
```


**Внимание:**
Это лишь позволяет работать в совместимом режиме, но всё ещё требует обложиться six.ensure_ функциями. Пример, на котором всё равно всё взрывается:
PY3: b'my_binary_string' -> file
PY2: file -> 'my_binary_string' -> file
PY3: file -> 'my_binary_string' 
В итоге получаем не bytes, а str, и попытка decode бросит исключение.

### SANDBOX

1) Начиная с июля sandbox поддержал [SANDBOX_PY3_TASK](https://clubs.at.yandex-team.ru/sandbox/3267).
2) Если вы используете [бинарные таски](https://wiki.yandex-team.ru/sandbox/tasks/binary/) - обязательно пишите библиотечную часть как PY23_LIBRARY, если они используются в SANDBOX_TASK и как PY3_LIBRARY, если они используются в SANDBOX_PY3_TASK.


## Благодарности

	* кто:inkoit [этушка](https://clubs.at.yandex-team.ru/arcadia/23044/23056#reply-arcadia-23056)
	* кто:idlesign [этушка](https://clubs.at.yandex-team.ru/arcadia/23825/23827#reply-arcadia-23827)
