## Правила оформления кода { #style-guide }

Для добавления кода в единый репозиторий, он **обязательно** должен следовать стандарту [PEP-8](https://legacy.python.org/dev/peps/pep-0008/) с некоторыми дополнениями:
1. Длина строки ограничена **200 символов**;
2. Запрещено использовать символ **табуляции для отступов**.

Следующие правила оформления кода не являются обязательными, но очень рекомендуются.

### Переносы строк { #line-break }

* Предпочтительный способ переноса длинных строк — перенос строки после обычной, квадратной или фигурной скобки. В случае необходимости можно добавить еще одну пару скобок вокруг выражения. Постарайтесь сделать правильные отступы для перенесённой строки.

* Предпочтительнее вставить перенос строки после бинарного оператора, но не перед ним.

```python
class Rectangle(Blob):
    def __init__(
        self,
        width, height,
        color='black', emphasis=None, highlight=0,
    ):
        if width == 0 and height == 0 and color == 'red' and emphasis == 'strong' or highlight > 100:
            raise ValueError('sorry, you lose')

        if width == 0 and height == 0 and (color == 'red' or emphasis is None):
            raise ValueError(
                "I don't think so -- values are {}, {}".format(width, height)
            )

        Blob.__init__(
            self,
            width, height,
            color=color, emphasis=emphasis, highlight=highlight,
        )
```

### Относительные импорты { #relative-import }

* Относительные импорты крайне не рекомендуются. Всегда указывайте абсолютный путь к модулю для всех импортов. Абсолютные импорты лучше переносимы и читабельны.

* Даже если модули находятся в одном пакете, используйте полное имя пакета при импортировании. Это позволит избежать нежелательного импорта модулей пакета дважды.

### Импортирование символов { #import-symbols }

Используйте оператор `import` **только** для пакетов и модулей. Никогда не импортируйте символы из других модулей. При таком подходе легче применять механизм подмены символов (monkey patching) для модульного тестирования.

* Используйте конструкции вида `from x import y`, где `x` -- имя пакета, а `y` -- имя модуля в этом пакете без префикса.
* Используйте конструкции вида `from x import y as z`, если в пространстве имен вашего модуля присутствуют два модуля `y`, или если `y` - слишком длинное имя.

К примеру, модуль `sound.effects.echo` может быть импортирован следующим образом:

```python
from sound.effects import echo
...
echo.EchoFilter(input, output, delay=0.7, atten=4)
```

### Операторы { #operators }

Никогда не используйте многоблочные операторы в одной строке с операциями:

**Правильно:**
```python
if foo == 'blah':
    do_blah_thing()
else:
    do_non_blah_thing()
try:
    something()
finally:
    cleanup()
```

~~Неправильно:~~
```python
if foo == 'blah': do_blah_thing()
else: do_non_blah_thing()
try: something()
finally: cleanup()
do_one(); do_two(); do_three(long, argument,
    list, like, this)
if foo == 'blah': one(); two(); three()
```

### Комментарии и документация { #comments-documentation }

#### Строки комментариев в коде { #comments }

```python
# try to book something with retries
for _ in xrange(5):
    something = Something()
    try:
        book(something)
    except BookingException:
        sleep(120)
```

Старайтесь вообще не использовать подобные комментарии.

Если вы испытываете желание описать детали фунционирования некоторого блока кода, имеет смысл оформить этот блок в отдельную функцию и описать функцию блоком описания. Если вы хотите выделить некоторую ключевую точку исполнения алгоритма — имеет смысл вызвать методы логирования.

#### Использование разметки { #docblock }

Вся документация, как и комментарии к публичным свойствам класса, должна быть размечена в машиночитаемом формате [Google DocString](https://google.github.io/styleguide/pyguide.html#38-comments-and-docstrings) или [Sphinx](http://sphinx-doc.org/contents.html). Это позволит вам легко перейти к автоматической генерации документации по вашему коду, а также позволит внешним людям пользоваться вашим API для интеграции. В каждом блоке документации к функции или методу класса, нужно указать назначение данной функции, описать ее аргументы, что она возвращает и классы генерируемых исключений:

Пример в формате Google DocString:
```python
def function_with_types_in_docstring(param1, param2):
    """Example function with types documented in the docstring.

    Args:
        param1 (int): The first parameter.
        param2 (str): The second parameter.

    Returns:
        bool: The return value. True for success, False otherwise.

    """


def function_with_pep484_type_annotations(param1: int, param2: str) -> bool:
    """Example function with PEP 484 type annotations.

    Args:
        param1: The first parameter.
        param2: The second parameter.

    Returns:
        The return value. True for success, False otherwise.

    """
```

Пример в Sphinx формате:
```python
def save(
    self,
    force_insert=False, validate=True, clean=True,
    write_concern=None, cascade=None, cascade_kwargs=None, _refs=None,
    **kwargs
):
    """
    Save the :class:`~mongoengine.Document` to the database. If the document already exists,
    it will be updated, otherwise it will be created.

    :param force_insert:    only try to create a new document, don't allow updates of existing documents.
    :param validate:        validates the document; set to ``False`` to skip.
    :param clean:           call the document clean method, requires `validate` to be `True`.
    :param write_concern:   Extra keyword arguments are passed down to
        :meth:`~pymongo.collection.Collection.save` OR
        :meth:`~pymongo.collection.Collection.insert`
        which will be used as options for the resultant ``getLastError`` command.
        For example,
        ``save(..., write_concern={w: 2, fsync: True}, ...)`` will wait until at least two servers
        have recorded the write and will force an fsync on the primary server.
    :param cascade:         Sets the flag for cascading saves.  You can set a default by setting
                            "cascade" in the document __meta__.
    :param cascade_kwargs:  (optional) kwargs dictionary to be passed throw to cascading saves.
                            Implies ``cascade=True``.
    :param _refs:           A list of processed references used in cascading saves.
    :return:                `None`

    :raises `~mongoengine.errors.ValidationError`:  Raised in case of document validation fails.
    :raises `~mongoengine.errors.OperationError`:   Raised on internal database operation error.
    :raises `~mongoengine.errors.NotUniqueError`:   Raised in case of primary key validation failed.

    .. versionchanged:: 0.8
        Cascade saves are optional and default to `False`.  If you want fine grain control then you can
        turn off using document ``meta['cascade'] = True``.  Also you can pass different kwargs to
        the cascade save using cascade_kwargs which overwrites the existing kwargs with custom values.
    """

```

### Работа с коллекциями { #collections }

#### Строки { #strings }
Используйте метод строки `format()`, `f''` строки, или оператор форматирования `%` (применяется для логирования), даже если все аргументы функции форматирования тоже строки.

**Правильно:**
```python
x = a + b
x = '{}, {}!'.format(imperative, expletive)
logger.debug('%s, %s!', imperative, expletive)
x = 'name: {}; score: {}'.format(name, n)
logger.debug('name: %s; score: %d', name, n)
```

~~Неправильно:~~
```python
x = '%s%s' % (a, b)  # use + in this case
x = '{}{}'.format(a, b)  # use + in this case
x = imperative + ', ' + expletive + '!'
logger.debug('{}, {}!'.format(imperative, expletive))
x = 'name: ' + name + '; score: ' + str(n)
logger.debug('name: %s; score: %d' % (name, n))
```

**Никогда** не форматируйте строки для логирования, всегда передавайте строку форматирования и аргументы шаблона отдельно. Таким образом можно избежать ненужных затрат вычислительных мощностей на форматирование строк, которые не попадут в лог, например, потому что выбран неподходящий для уровень логирования.

Избегайте использования операторов `+` и `+=` для компиляции строки в цикле. Т.к. строки являются неизменяемыми (immutable) объектами, подобные конструкции могут привести к порождению ненужных временных объектов, что может привести к квадратичному времени исполнения вместо линейного. Чтобы избежать этого, добавляйте каждую подстроку в список, чтобы в результате объединить их посредством вызова `''.join()`, или используйте модифицируемые буферы, такие как `StringIO`.

**Правильно:**
```python
items = ['<table>']
for last_name, first_name in employee_list:
    items.append('<tr><td>%s, %s</td></tr>' % (last_name, first_name))
items.append('</table>')
employee_table = ''.join(items)
```

~~Неправильно:~~
```python
employee_table = '<table>'
for last_name, first_name in employee_list:
    employee_table += '<tr><td>%s, %s</td></tr>' % (last_name, first_name)
employee_table += '</table>'
```

Используйте только один тип символа строковой кавычки (`"` или `'`) для всего файла исходного кода. однако изменение единого стиля вполне нормально для того, чтобы избежать использования escape-символа `\`.

**Правильно:**
```python
Python('Why are you hiding your eyes?')
Gollum("I'm scared of lint errors.")
Narrator('"Good!" thought a happy Python reviewer.')
```

~~Неправильно:~~
```python
Python("Why are you hiding your eyes?")
Gollum('The lint. It burns. It burns us.')
Gollum("Always the great lint. Watching. Watching.")
```

Используйте `"""` для многострочных констант вместо `'''`. Используйте `"""` только для оформления блоков документации.

#### List Comprehensions { #list-comprehensions }

Использование конструкторов списков (list comprehensions) допустимо для простых случаев. Конструкторы списков и выражения-генераторы (generator expressions) предоставляют простой и эффективный способ для создания списков и итераторов без сортировки и фильтрации их посредством вызовов функций типа `map()`, `filter()`, `lambda`-выражений и т.д. Простые конструкторы списков могут выглядеть гораздо проще и понятней, чем использование каких-либо операций над списками. Выражения-генераторы могут быть весьма эффективными, так как они не создают весь список сразу. Переусложненные конструкторы списков могут быть сложны для чтения и понимания.

**Правильно:**
```python
result = []
for x in range(10):
    for y in range(5):
        if x * y > 10:
            result.append((x, y))

for x in xrange(5):
    for y in xrange(5):
        if x != y:
            for z in xrange(5):
                if y != z:
                    yield (x, y, z)
```

~~Неправильно:~~
```python
result = [(x, y) for x in range(10) for y in range(5) if x * y > 10]

return (
    (x, y, z)
    for x in xrange(5)
    for y in xrange(5)
    if x != y
    for z in xrange(5)
    if y != z
)
```

#### Итераторы и операторы по умолчанию { #iterators }

Используйте итераторы и операторы по-умолчанию для тех типов, которые поддерживают их (`list`, `dict`, `file` и так далее).

**Правильно:**
```python
for key in adict: ...
if key not in adict: ...
if obj in alist: ...
for line in afile: ...
for k, v in dict.items(): ...
```

~~Неправильно:~~
```python
for key in adict.keys(): ...
if not adict.has_key(key): ...
for line in afile.readlines(): ...
```
