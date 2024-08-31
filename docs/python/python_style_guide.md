## Требования:

Требования проверяются линтерами и обязательны.
Следуем [PEP-8](https://legacy.python.org/dev/peps/pep-0008/) со следующими дополнениями:
  - Длина строки 120 символов.
  - Запрещено использовать символ табуляции для отступов.

В Аркадии в качестве линтере используем [Flake8](https://flake8.pycqa.org/en/latest/index.html#) + плагины с единым конфигом на всех ([конфиг](https://a.yandex-team.ru/arc/trunk/arcadia/build/config/tests/flake8/flake8.conf)).

## Рекомендации

Рекомендации не проверяются линтерами, но следовать им желательно.

### Внешний вид кода

#### Перенос строк

Предпочтительный способ переноса длинных строк — использование подразумевающегося продолжения строки между обычными, квадратными и фигурными скобками. В случае необходимости можно добавить еще одну пару скобок вокруг выражения. Постарайтесь сделать правильные отступы для перенесённой строки.

Предпочтительнее вставить перенос строки после бинарного оператора, но не перед ним.

Вот несколько примеров:
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

#### Относительные импорты

Относительные импорты крайне не рекомендуются — всегда указывайте абсолютный путь к модулю для всех импортирований. Даже сейчас, когда [PEP-328](http://legacy.python.org/dev/peps/pep-0328/) реализован в версии Python 2.5, использовать явные относительные импорты нежелательно, потому что абсолютные импорты лучше переносимы и читабельны.

Даже если модули находятся в одном пакете, используйте полное имя пакета при импортировании. Это позволит избежать нежелательного импорта модулей пакета дважды.

#### Импортирование символов

Используйте оператор `import` __только__ для пакетов и модулей. Никогда не импортируйте символы других модулей. Это хороший механизм для переиспользования кода в других модулях и программах. Дополнительно, в этом случае легче применять механизм подмены символов (monkey patching) для модульного тестирования.

Использование различных пространств имен очень просто. Исходный код каждого идентификатора указывает на его происхождение: `x.Obj` показывает, что `Obj` определен в модуле `x`. Однако, имена модулей по-прежнему могут пересекаться, а имена некоторых модулей непозволительно длинные.

Для решения этого можно использовать следующие разрешения:

  * Используйте конструкции вида `from x import y`, где `x` -- имя пакета, а `y` -- имя модуля в этом пакете без префикса.
  * Используйте конструкции вида `from x import y as z`, если в пространстве имен вашего модуля присутствуют два модуля `y`, или если `y` -- непозволительно длинное имя.

К примеру, модуль `sound.effects.echo` может быть импортирован следующим образом:
```python
from sound.effects import echo
...
echo.EchoFilter(input, output, delay=0.7, atten=4)
```

#### Прочие рекомендации

Никогда не используйте многоблочные операторы в одной строке с операциями:

**правильно:**
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

~~неправильно:~~
```python
if foo == 'blah': do_blah_thing()
else: do_non_blah_thing()
try: something()
finally: cleanup()
do_one(); do_two(); do_three(long, argument,
    list, like, this)
if foo == 'blah': one(); two(); three()
```

#### Строки комментариев в коде

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

Если вы испытываете желание описать детали фунционирования некоторого блока кода, имеет смысл оформить этот блок в отдельную функцию и описать функцию блоком описания (см. ниже). Если вы хотите выделить некоторую ключевую точку исполнения алгоритма — имеет смысл использования функций журналирования (logging).

### Строки документации

#### Использование разметки

Вся документация, как и комментарии к публичным свойствам класса, должна быть размечена в машиночитаемом формате [Google docstring](https://google.github.io/styleguide/pyguide.html#38-comments-and-docstrings) или [Sphinx](http://sphinx-doc.org/contents.html), даже если вы не предполагаете генерацию документации по исходному коду в ближайшее время. Это позволит вам легко перейти к автоматической генерации документации по вашему коду, а также позволит внешним людям, по отношению к вашему проекту, пользоваться вашим API для интеграции.

Помните, что в каждом блоке документации к функции или методу класса, вы должны указать назначение данной функции, описать ее аргументы, а также указать, что она возвращает. Указание на классы генерируемых исключений также будет совсем не лишним. Ниже приведен пример документации в Google docstring формате:

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

А вот пример в Sphinx формате:

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

##### Какой формат выбрать?

Тут нет никаких рекомендаций, используйте тот формат, с которым вам комфортнее работать. Современные IDE хорошо понимают оба формата (на Sphinx были жалобы в VS Code).

### Работа с коллекциями

#### Строки
Используйте метод строки `format()`, f'' строки, или оператор форматирования "%" (только в случае форматирования сообщений в журнал (logging)), даже если все аргументы функции форматирования тоже строки.

**правильно:**
```python
x = a + b
x = '{}, {}!'.format(imperative, expletive)
logger.debug('%s, %s!', imperative, expletive)
x = 'name: {}; score: {}'.format(name, n)
logger.debug('name: %s; score: %d', name, n)
```

~~неправильно:~~
```python
x = '%s%s' % (a, b)  # use + in this case
x = '{}{}'.format(a, b)  # use + in this case
x = imperative + ', ' + expletive + '!'
logger.debug('{}, {}!'.format(imperative, expletive))
x = 'name: ' + name + '; score: ' + str(n)
logger.debug('name: %s; score: %d' % (name, n))
```

Отдельного внимания заслуживает журналирование. **Никогда** не форматируйте строки для функций журналирования, всегда передавайте строку форматирования и аргументы шаблона отдельно. Это связано с тем, что таким образом можно избежать ненужных затрат вычислительных мощностей на форматирование строк, которые не попадут в журнал по причине, например, неподходящего для журнала уровня журналирования.

Избегайте использование операторов `+` и `+=` для компиляции строки в цикле. Т.к. строки являются неизменяемыми (immutable) объектами, подобные конструкции могут привести к порождению ненужных временных объектов, что может привести к квадратичному времени исполнения вместо линейного. Чтобы избежать этого, добавляйте каждую подстроку в лист, чтобы в результате объединить их посредством вызова `''.join()`, или используйте модифицируемые буферы, такие как `StringIO`.

**правильно:**
```python
items = ['<table>']
for last_name, first_name in employee_list:
    items.append('<tr><td>%s, %s</td></tr>' % (last_name, first_name))
items.append('</table>')
employee_table = ''.join(items)
```

~~неправильно:~~
```python
employee_table = '<table>'
for last_name, first_name in employee_list:
    employee_table += '<tr><td>%s, %s</td></tr>' % (last_name, first_name)
employee_table += '</table>'
```

Определяйтесь с использованием символа строковой кавычки (`"` или `'`) один раз для всего файла исходного кода, однако изменение единого стиля вполне нормально для того, чтобы избежать использования escape-символа `\`.

**правильно:**
```python
Python('Why are you hiding your eyes?')
Gollum("I'm scared of lint errors.")
Narrator('"Good!" thought a happy Python reviewer.')
```

~~неправильно:~~
```python
Python("Why are you hiding your eyes?")
Gollum('The lint. It burns. It burns us.')
Gollum("Always the great lint. Watching. Watching.")
```

Используйте `"""` для многострочных констант вместо `'''`. Используйте исключительно `"""` для оформления блоков документации.

#### List Comprehensions

Использование конструкторов списков (list comprehensions) вполне нормально для простых случаев.

Конструкторы списков и выражения-генераторы (generator expressions) предоставляют простой и эффективный способ для создания списков и итераторов без сортировки и фильтрации их посредством вызовов функций типа `map()`, `filter()`, `lambda`-выражений и т.д. Простые конструкторы списков могут выглядеть гораздо проще и понятней, чем использование каких-либо операций над списками. Выражения-генераторы могут быть весьма эффективными, так как они не создают весь список сразу. Однако, переусложненные конструкторы списков могут быть сложны для чтения и понимания.

**правильно:**
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

~~неправильно:~~
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

#### Итераторы и операторы по умолчанию

Используйте итераторы и операторы по умолчанию для тех типов, которые поддерживают их, таких как `list`, `dict`, `file` и так далее.

Типы-контейнеры (container types), такие как словари и списки, определяют итераторы и списковые операторы, такие как `in`, `not in` и так далее. Использование таких итераторов и операторов является простым и эффективным. Вызовы к подобным методам происходят напрямую, минуя лишние вызовы методов объектов этих типов.

**правильно:**
```python
for key in adict: ...
if key not in adict: ...
if obj in alist: ...
for line in afile: ...
for k, v in dict.items(): ...
```

~~неправильно:~~
```python
for key in adict.keys(): ...
if not adict.has_key(key): ...
for line in afile.readlines(): ...
```

*Для добавления исключения в любое из правил напишите на python-com@.*
