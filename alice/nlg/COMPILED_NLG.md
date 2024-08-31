# Новый компилируемый NLG

Мы задепрекейтили NLG-сервис. Теперь Megamind будет выполнять NLG-шаблоны с помощью специального компилятора, превращающего их в код на C++. Этот код компилируется в библиотеку `alice/nlg/config` и используется Мегамайндом автоматически.

Мы метим в широкую совместимость со старым NLG, основанным на Jinja2. Вот документация по ним:
- https://jinja.palletsprojects.com/en/2.10.x/templates/
- https://wiki.yandex-team.ru/assistant/guides/developers-guide/#sintezreplikbota
- https://github.yandex-team.ru/vins/vins-dm/pull/1253

## Как начать пользоваться

Если ваш шаблон уже лежит в `alice/nlg/config`, вам достаточно сделать следующее:
- прописать его (и шаблоны, которые он импортирует) в `alice/nlg/config/ya.make`. Теперь `alice/nlg/config` — это библиотека, а не просто папка с шаблонами!
- убедиться, что он также прописан в `alice/nlg/bin/ya.make`, причем по тем же правилам, что и шаблоны `sound`
  - имя ресурса должно получаться из имени файла заменой `../config` на `/nlg/config`, например: `../config/sound/sound_mute_ru.nlg /nlg/config/sound/sound_mute_ru.nlg`
- откомпилировать `alice/nlg`. После этого вы сможете использовать команду `alice/nlg/tool/tool render`, чтобы протестировать работу вашего шаблона (обязательно это сделайте!)

После этих шагов Megamind будет автоматически использовать компилируемую версию вашего шаблона и перестанет ходить за ним в NLG-сервис. Но помните, что пока что в Megamind есть флаг эксперимента `mm_disable_compiled_nlg`, и при его включении Megamind начнет ходить в NLG-сервис за всеми шаблонами. Чтобы ваш сценарий при этом не перестал работать, вы должны добавить свои шаблоны как в библиотеку `alice/nlg/config`, так и в бинарь NLG-сервиса `alice/nlg/bin`.

В качестве примера для подражания можно использовать `alice/nlg/sound`.

Если ваш шаблон лежит в `alice/vins/apps/personal_assistant/personal_assistant/config`, то вам стоит подумать о том, чтобы скопировать его в `alice/nlg/config`. В будущем мы возможно научимся компилировать и использовать в Megamind шаблоны из нескольких локаций. Сейчас это осложняется тем, что очень многие шаблоны требуют допиливания при переносе — как из-за различий между VINS и Megamind, так и из-за неполной поддержки фич компилируемыми шаблонами.

## Мой шаблон не поддерживается, что делать?

Ниже вы найдете список отличий легаси-NLG от компилируемого NLG. Некоторые из них можно устранить (например, вам нужна какая-то встроенная функция или метод), некоторые неустранимы (например, использование динамических `varargs` и `kwargs` в макросах). В любом случае можно обсудить проблему с a-square@ и найти подходящее решение.

## Отличия от легаси-NLG

Отличий много, но большая их часть не затрагивает поведение существующих шаблонов, так как одна из главных целей при разработке нового NLG — совеместимость с уже написанными шаблонами.

### Модель запуска

Расмотрим вымышленный шаблон `foo.nlg`:

```
{% set x = 1 %}
{% import 'bar.nlg' as bar with context %}
{% set y = 2 %}
{% nlgimport 'baz.nlg' %}
{% phrase render_me %}
  {% chooseline %}
    Hello
    Goodbye
  {% endchooseline %}
{% endphrase %}
```

В **легаси-NLG** его обработка происходит так:
- `x = 1`
- выполняем тело `bar.nlg`, ему будет доступна переменная `x` (но не `y`). Проинициализированный модуль привязываем к переменной `bar`
- `y = 2`
- выполняем тело `baz.nlg`
- объявляем питончью функцию с именем `render_me` (по сути присваиваем этой переменной ссылку на функцию)
- затем специальный код обходит фразы и карточки в этом шаблоне, а также в шаблоне `baz.nlg`, и кладет ссылки на них в специальный словарь
- из словаря берется и запускается нужная фраза или карточка.

Этот процесс инициализации происходит при каждом запросе в NLG-сервис, поэтому в легаси-NLG при иницилиазиции вы можете использовать переменные `context`, `form` и `req_info`, которые заполняются в конкретном запросе к сервису. Например вы могли бы заменить `y = 2` выше на `y = req_info.z`, и это бы работало.

В компилируемом **NLG обработка** происходит совсем по-другому!

Сначала содержимое файла "сортируется" — разделяется на последовательные секции:
```
# секция импортов
{% import 'bar.nlg' as bar with context %}
{% nlgimport 'baz.nlg' %}
```
```
# секция инициализации
{% set x = 1 %}
{% set y = 2 %}
```
```
# секция фраз, карточек и макросов
{% phrase render_me %}
  {% chooseline %}
    Hello
    Goodbye
  {% endchooseline %}
{% endphrase %}
```

Затем при инициализации среды исполнения прикапывается информация:
- `import 'bar.nlg' as bar with context`
- `x = 1`
- `y = 2`
- рекурсивно замораживаем значения всех глобальных переменных (чтобы не волноваться о гонках)
- регистрируем все фразы и карточки из `baz.nlg` в текущем модуле
- регистрируем фразу `render_me` в текущем модуле (на этот момент она уже откомпилирована в C++-функцию)

Важно запомнить, что `with context` работает не так, как раньше:
- раньше при инициализации модуля он имел доступ к заполненным на тот момент переменным импортирующего модуля (то есть при инциализации контекст импортируемого модуля был не пустым, а shallow-копией контекста импортирующего модуля)
- теперь он не имеет этого доступа, но объявленные в нем фразы, карточки и макросы будут видеть все глобальные переменные импортирующего модуля при выполнении

На практике сейчас в NLG `with context` используется только для того, чтобы передать доступ к переменным `context`, `req_info` и `form`, а они теперь в любом случае не видны при инициализации глобальных переменных, поэтому это различие не так существенно, как можно подумать.

Но сам тот факт, что к переменным запроса больше нет доступа при инициализации глобальных переменных, потребует переписывания некоторых шаблонов! Например, вот такой шаблон:
```
{% set foo = context.foo %}
{% set bar = context.bar %}
{% phrase test %}
  {{ foo }} {{ bar }}
{% endphrase %}
```

...придется [переписать](config/example/init_idiom.nlg):

```
{% macro init(ns) %}
  {% set ns.foo = context.foo %}
  {% set ns.bar = context.bar %}
{% endmacro %}

{% phrase test %}
  {% set ns = namespace() %}
  {% do init(ns) %}
  {{ ns.foo }} {{ ns.bar }}
{% endphrase %}
```

Это чуть менее удобно, но зато позволяет компилируемым NLG-шаблонам выполнить всю инициализацию, не требующую знаний о запросе, заранее, а при рендеринге фраз и карточек выполнять только реально нужные инструкции.

### Объектная модель

Легаси-NLG использовал объектную модель Python 2.7. Она очень мощная и динамичная, поэтому конечно мы не стали воссоздавать ее в C++. Поскольку NLG-сервис получает данные запроса в JSON-формате, новая модель данных тоже очень похожа на JSON и состоит из следующих типов объектов:
- `undefined`
  - Его можно печатать, итерироваться по нему и получать его булевое значение — `false`. Все остальные операции приводят к исключению.
- `bool`
- `integer`
  - в питоне есть два целочисленных типа: 64-битный `int` прозрачно переходит в не ограниченный по длине `long`
  - мы поддерживаем только знаковые 64-битные числа
- `double`
  - мы не поддерживаем бесконечные числа (`inf`, `-inf`, `nan`). Операции, приводящие к ним (например, деление на ноль), кидают исключения
- `string`
  - в легаси-NLG хранил юникодные строки с поддержкой быстрого обращения по индексу (`string[i]` имел сложность O(1))
  - в новом NLG хранит UTF-8 строки без поддержки индексирования (можно добавить в будущем). Каждый символ строки знает, является ли он частью текстового и/или голосового выхлопа.
- `list`
  - список каких-то других значений
- `dict`
  - словарь с ключами-строками; в отличие от строк-значений, эти строки не имеют информации о тексте/голосе
- `range`
  - полностью эмулирует питонячий `xrange`
- `none`

В нашей объектной модели у объектов не может быть атрибутов, но значения словрая можно получить через точку. В частности, нет никаких различий между `dict` и `namespace`, мы считаем эти два понятия идентичными. Для любого словаря `xs` можно сказать `{% set xs.foo = 2 %}` и это не приведет к ошибке.

Также мы неявно конвертируем `tuple` в `list`. Предполагается, что если для вашей логики важна гарантия иммутабельности `tuple`, то она сложная и ее правильно вынести из шаблона в сценарий Megamind.

Наконец, функции и переменные существуют в "параллельных мирах": мы не поддерживаем вызов содержимого переменной как будто это функция, и не поддерживаем присваивания переменным ссылок на макросы и другие функции.

### Воспроизводимость запуска

Запуск компилируемых NLG-шаблонов в Megamind воспроизводим. Это значит, что каждый запрос использует свой собственный PRNG, и его сид берется из запроса. Также это значит, что при инициализации глобальных переменных используется отдельный PRNG [с жестко заданным сидом](https://xkcd.com/221/).

### Другие отличия

- `form.raw_form` — обычный словарь, в нем есть список `slots` и словарь `slots_by_name`
- специальный вызов `caller` может принимать только позиционные аргументы
- импорты резолвятся на этапе компиляции, поэтому:
  - нельзя импортировать неконстантный шаблон: `{% import some_var as foo %}` приведет к ошибке
  - импорты не могут перетирать друг друга: `{% import 'a' as foo %}{%import 'b' as foo %}` приведет к ошибке
- как уже было замечено выше, строки не считаются последовательностями символов, но с ними можно выполнять обычные операции (разбивать, стрипать, тримить...). Возможно в будущем мы поддержим для строк random access-операции, но сейчас их нет, т.к. при хранении текста в UTF-8 они будут иметь сложность O(1) при наивной реализации, а перекодировать строки в UTF-8, UTF-16 или UTF-32 в зависимости от наличия в них определенных символов (как в Python) — дорого.
- также в отличие от легаси-NLG, в новом NLG у строк нет флагов эскейпинга, им мы не поддерживаем встроенные фильтры вроде `escape` (но поддерживаем наш собственный фильтр `html_escape`)
- печать `double` может отличаться от питончей. Мы не добавляем `.0` при выводе целых `double`-значений, и для конверсии в строку мы используем алгоритм Grisu3 (как в Python 3), он в некоторых случаях печатает более короткие строковые представления, чем Python 2.7
- операторы сравнения чаще кидают исключения
  - операторы `==` и `!=` могут сравнивать все
  - операторы `<`, `<=`, `>`, `>=` кидают исключение при попытке сравнить несравнимые типы (по аналогии с Python 3)
- разделение голоса и текста реализовано по-другому
  - в легаси-NLG используются xml-теги, которые вырезаются регулярными выражениями при пост-процессинге, неявно вложенные теги (что возможно, например, при вызове макроса) приводят к ошибке
  - в новом NLG (логически) у каждого символа строки есть флаги наличия символа в голосе и тексте, при пост-процессинге в текст/голос попадают только символы с соответствующими флагами. Вложенные теги текста/голоса обрабатываются "субтрактивно" (text внутри voice ни попадет никуда), большинство функций обработки строк уважают теги, но некоторые могут вести себя странно, в этом случае нужно поговорить с разработчиками и возможно поправить
    - в частности, сейчас фильтр `emojize` сбрасывает теги в дефолтное положение, обычно это не вызывает проблем (`{%tx%}{{ ':x:' | emojize }}{%etx%}` будет работать ожидаемо)
    - `chooseline` кидает исключение, если его строки разбиваются посредине тега `text` или `voice`. Это by design, мы считаем такую ситуацию ошибкой и явно проверяем
  - `chooseline` не поддерживает `no_repeat`, `cycle`. Мы считаем, что это поведение должно быть реализовано на уровне сценария, но возможно мы неправы :)
  - не поддерживаются проверки  `is callable`, `is escaped`, `is sameas(other)`
- импортируемые модули бывают двух видов: внешние и внутренние.
  - внутренние: модули, прописанные в макросе `COMPILE_NLG`, т.е. модули самой библиотеки. Для импорта используются команды `import`, `from`, `nlgimport`
  - внешние: не прописаны в `COMPILE_NLG`, т.е. являются модулями других NLG-библиотек. Данные библиотеки должны быть указаны в `PEERDIR`. Для импорта используются команды `ext_import`, `ext_from`, `ext_nlgimport`
- значения по умолчанию при определении макроса могут быть только следующих типов: `list`, `dict`, `range`, `int`, `float`, `bool`; передача глобальной переменной в качестве значения по умолчанию: `{% set x=5 %} {% macro foo(value=x) %}` приведёт к ошибке