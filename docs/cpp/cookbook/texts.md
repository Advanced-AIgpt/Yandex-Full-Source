# Hitchhiker's Guide to C++ in Arcadia: Texts

![](../img/dont_panic.png)

## Сплит строк

Для сплита строк используется объект `StringSplitter`. Подключение:

```c++
#include <util/string/split.h>
```

Вкратце сплит строк выглядит как:

```c++
StringSplitter(<объект сплита>)
    .Split(<объект-разделитель>)
    .<дополнительные условия>()
    .<фильтры>()
    .<обработка результата>();
```

Поддерживаются стандартные и аркадийные строки, обычные символы, UTF-16 / UTF-32 версии.

Подробное описание:

### Объект сплита

* `StringSplitter(value)`, где `value` — строкоподобный объект, например:
  * `TString`
  * `TStringBuf`
  * `std::string`
  * `std::string_view`
  * `const char*` (null-terminated)
* `StringSplitter(begin, end)`, где `begin`, `end` являются:
  * указателями `char*`
  * или итераторами у строкоподобных объектов
* `StringSplitter(const char* ptr, size_t size)`

### Объект-разделитель

* `Split(Char sym)` — сплит по обычному символу. Пример:

  ```c++
  StringSplitter("ab,b,cd").Split(',');
  Result:
  {"ab", "b", "cd"}
  ```

* `SplitBySet(Char* set)` — сплит по множеству (сплит по каждому символу), где `set` является null terminated строкой из символов. Пример:

  ```c++
  StringSplitter("abyandexcd").SplitBySet("abc");
  Result:
  {"", "", "y", "ndex", "d"}
  ```

* `SplitByString(StringBuf buf)` — сплит по строке, где `buf` — соответствующий view на строку или умеющий конструироваться в это (например, указатель). Пример:

  ```c++
  StringSplitter("hellohowworldhowdanlark").SplitByString("how");
  Result:
  {"hello", "world", "danlark"}
  ```

* `SplitByFunc(callable)` — сплит по функции. А именно: на каждый символ вызывается `callable` и, если возвращается `true`, этот символ считается разделителем и не входит в ответ. Пример:

  ```c++
  StringSplitter("123 456 \t\n789\n10\t 20")
      .SplitByFunc(
          [](char a) {
              return a == ' ' || a == '\t' || a == '\n';
          }
      );

  Result:
  {"123", "456", "", "", "789", "10", "", "20"}
  ```

### Дополнительные условия

По умолчанию `StringSplitter` **будет** сохранять пустые строки:

  ```c++
  StringSplitter(TString()).Split(',');
  Result:
  {""}

  StringSplitter("wwwww").SplitByString("ww");
  Result:
  {"", "", "w"}
  ```

Для изменения этого поведения можно указывать дополнительные условия, которые разделены на 2 группы:

#### Стоп-условия

* `Take(size_t count)` — возьмём `count` значений ответа. Если значений меньше, чем `count`, возьмём все. Примеры:

  ```c++
  StringSplitter("1,2,3,4,5").Split(',').Take(3);
  Result:
  {"1", "2", "3"}

  StringSplitter("1@@2@@3@@4@@5").SplitByString("@@").Take(100500);
  Result:
  {"1", "2", "3", "4", "5"}
  ```

* `Limit(size_t count)` — возьмём первые `count - 1` поспличенных токенов, в последний токен отправим оставшуюся строку. Если токенов меньше `count`, эквивалентно обычному сплиту. Примеры:

  ```c++
  StringSplitter("1,2,3,4,5").Split(',').Limit(3);
  Result:
  {"1", "2", "3,4,5"}

  StringSplitter("1@@2@@3@@4@@5").SplitByString("@@").Limit(100500);
  Result:
  {"1", "2", "3", "4", "5"}
  ```

#### Фильтры

* `SkipEmpty()` — не берём в ответ пустые строки. Пример:

  ```c++
  StringSplitter("1,,,4,5").Split(',').SkipEmpty();
  Result:
  {"1", "4", "5"}
  ```

Фильтры и стоп-условия можно чередовать друг с другом, притом порядок применения будет идти слева направо. То есть:
* `SkipEmpty().Take(5)` означает пропуск всех пустых токенов и сохранение в ответ 5 непустых;
* `Take(5).SkipEmpty()` означает рассмотрение первых пяти токенов, а потом фильтрация по пустым токенам.

Примеры:

```c++
StringSplitter("1,,,4,5").Split(',').SkipEmpty().Take(2);
Result:
{"1", "4"}

StringSplitter("1,,,4,5").Split(',').SkipEmpty().Take(20).Take(2);
Result:
{"1", "4"}

StringSplitter("1,,,4,5").Split(',').Take(3).SkipEmpty();
Result:
{"1"}

StringSplitter("1aaba2bbababa3a4b5").SplitBySet("ab").SkipEmpty().Limit(3).Take(3);
Result:
{"1", "2", "3a4b5"}
```

### Обработка и сохранение результата

* Итераторы

  ```c++
  for (TStringBuf token : StringSplitter(str).Split(',')) {
      DoSomething(token);
  }

  TVector<TStringBuf> v;
  auto s = StringSplitter("1 2 3 4 5").Split(' ').SkipEmpty();
  Copy(s.begin(), s.end(), std::back_inserter(v));
  Result:
  v == {"1", "2", "3", "4", "5"}

  TVector<TString> v(5);
  auto s = StringSplitter("1 2 3 4 5").Split(' ').SkipEmpty().Limit(5);
  Copy(s.begin(), s.end(), v.begin()); // std::back_inserter не скомпилируется!
  Result:
  v == {"1", "2", "3", "4", "5"}

  auto s = StringSplitter("1 2 3 4 5").Split(' ').SkipEmpty();
  TVector<TString> v{s.begin(), s.end()};
  Result:
  v == {"1", "2", "3", "4", "5"}

  for (auto&& it : StringSplitter(str).Split(',')) {
      it.Token() — сам токен
      it.Delim() — разделитель
      it.TokenStart() — начало токена
      it.TokenDelim() — начало разделителя и конец токена
      it.empty() — возвращает "пустой ли токен"
      operator bool() — эквивалентно !empty()
  }
  ```

* `Consume(callable)` — на каждую строку ответа вызовет `callable`. `callable` должен принимать `TStringBuf` или `std::string_view`. Если `callable` возвращает `bool`, то вызываться будет до первого возвращаемого `false`. Если `callable` возвращает `void`, просто вызовется от всех токенов. Другие возвращаемые типы не принимаются.

  ```c++
  ui32 cnt = 0;
  auto f = [&](const TStringBuf s) -> void {
      ui32 n = FromString<ui32>(s);
      if (n > 3) {
          ++cnt;
      }
  };
  StringSplitter("1 2 3 4 5").Split(' ').SkipEmpty().Consume(f);
  Result:
  cnt == 2


  TVector<TString> v;
  auto f = [&](const TStringBuf s) -> bool {
      ui32 n = FromString<ui32>(s);
      v.push_back(TString(s));
      if (n > 3) {
          return false;
      }
      return true;
  };
  StringSplitter("1 2 3 4 5").Split(' ').SkipEmpty().Consume(f);
  Result:
  v == {"1", "2", "3", "4"}
  ```

В какие объекты можно сохранять результат:

* В любой контейнер с методом `::emplace_back()`:

  ```c++
  TVector<TString> v = StringSplitter(TStringExample).Split(',');
  TVector<TStringBuf> v = StringSplitter(TStringExample).Split(',');

  std::vector<std::string> v = StringSplitter(StdStringExample).Split(',');
  std::vector<std::string_view> v = StringSplitter(StdStringExample).Split(',');
  ```

{% note info %}

Бонусом можно сохранять в векторы от `std::string` и в векторы от `std::string_view` аркадийные строки.

{% endnote %}

* В любой контейнер с методом `::emplace()`:

  ```c++
  THashSet<TStringBuf> v = StringSplitter(TStringExample).Split(',');
  ```

* `AddTo(Container*)` — функция для добавления в контейнер:

  ```c++
  TVector<TString> v = {"1", "2"};
  StringSplitter("1,2,3,4,5").Split(',').AddTo(&v);
  Result:
  v == {"1", "2", "1", "2", "3", "4", "5"}
  ```

* `Collect(Container*)` — функция для добавления в контейнер. Отличие от `AddTo` в том, что контейнер очищается перед добавлением. Также накладывает условие на контейнер, что в нём должна быть функция `::clear()`:

  ```c++
  TVector<TString> v = {"1", "2"};
  StringSplitter("1,2,3,4,5").Split(',').Collect(&v);
  Result:
  v == {"1", "2", "3", "4", "5"}
  ```

* `TryCollectInto(Args&&... args)`
* `CollectInto(Args&&... args)`

  Функции для сбора в элементы. Вызывают `FromString` от каждого элемента. `TryCollectInto` возвращает `false`, если не удалось распарсить все элементы в соответствующие типы (сейчас будут парситься все элементы, **не** до первого падения). `CollectInto` просто бросает исключение.

  ```c++
  int a = -1;
  TStringBuf s;
  double d = -1;
  StringSplitter("2 substr 1.02").Split(' ').CollectInto(&a, &s, &d);
  Result:
  a == 2,
  s == "substr",
  d == 1.02


  int a = -1;
  int s = 123;
  double d = -1;
  bool succ = StringSplitter("2 substr 1.02").Split(' ').TryCollectInto(&a, &s, &d);
  Result:
  succ == false
  a == 2,
  s == 123,
  d == 1.02

  int a = -1;
  int s = 123;
  double d = -1;
  StringSplitter("2 substr 1.02").Split(' ').CollectInto(&a, &s, &d);
  Result:
  Exception
  ```

* `ParseInto(Container*)` — добавляет в контейнер (в контейнере должна быть функция `::emplace_back()` или просто `::emplace()`), вызывая `FromString` от его `value_type`. Бросает исключение, если не удалось распарсить. Контейнер не очищается.

  ```c++
  TVector<int> answer = { 42 };
  StringSplitter("1 2 3 4").Split(' ').ParseInto(&answer);
  Result:
  answer == {42, 1, 2, 3, 4}


  StringSplitter("1 2    3 4").Split(' ').ParseInto(&answer);
  Result:
  Exception
  ```

Доп. методы:

* `Count()` — вернёт количество токенов в ответе.

  ```c++
  StringSplitter(" 1 ").Split(' ').Count();
  Result:
  3

  StringSplitter(" 1 ").Split(' ').SkipEmpty().Count();
  Result:
  1
  ```
