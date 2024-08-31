# Сборник коротких рецептов в вопросах и ответах

Здесь собраны рецепты решния типовых задач описания сборки. У нас есть отдельная страница [FAQ](../general/faq.md) с концептуальными вопросами о системе сборки и ответами на них.

## Q: У меня есть несколько файлов, я просто хочу собрать из них программу

**A:** напишите следующее

```
PROGRAM()

OWNER(username1 username2)

SRCS(
    file_one.cpp
    file_two.cpp
)

END()
```

и положите в каталог с проектом под именем `ya.make`. Затем необходимо включить проект в структуру проектов — для этого `ya.make` проекта более высокого уровня включите строчку

```
RECURSE(project)
```
либо

```
RECURSE_ROOT_RELATIVE(path/to/project)
```
где `path/to/project` — путь к проекту от корня аркадии

- Для кода на Python используйте `PY3_PROGRAM`
- Для кода на Java используйте `JAVA_PROGRAM`
- Для кода на Go используйте `GO_PROGRAM`


## Q: У меня есть несколько файлов, я просто хочу собрать из них библиотеку

**A:** Поступайте точно так же, но вместо [`PROGRAM()`](../manual/cpp/modules.md#program) используйте [`LIBRARY()``](../manual/cpp/modules.md#library), например

```
LIBRARY()

OWNER(username)

SRCS(
    file_one.cpp
    file_two.cpp
)

END()
```

- Для кода на Python используйте `PY3_LIBRARY`
- Для кода на Java используйте `JAVA_LIBRARY`
- Для кода на Go используйте `GO_LIBRARY`

## Q: Я просто хочу собрать проект (программу или библиотеку), но она зависит от других библиотек аркадии

**A:** Используйте [PEERDIR()](../manual/common/macros.md#peerdir). В качестве аргумента укажите проекты (как всегда путь от корня аркадии), от которых зависит ваш проект.

```
LIBRARY()

OWNER(username)

SRCS(
    file_one.cpp
    file_two.cpp
)

PEERDIR(
    ysite/yandex/numerator
)

END()
```

  * Не важно, собираете ли вы программу или библиотеку
  * `util` и его подкаталоги (кроме `draft` и `charset`) неявно присутствует в `PEERDIR` у `LIBRARY`, `PROGRAM` и `DLL`
  * `PEERDIR` транзитивен, то есть, если вы собрали библиотеку с `PEERDIR`, проекты, указавшие в `PEERDIR` вашу библиотеку, получат и ваш `PEERDIR`
  * **`PEERDIR` должен включать в себя ВСЕ используемые библиотеки (то есть, на любой #include должен быть `PEERDIR``)**

## Q: Я просто хочу собрать проект (программу или библиотеку), но мне не нравится имя бинарника

**A:** имя программы по умолчанию совпадает с именем каталога, а для библиотеки также включает её родителя и деда. Передайте новое имя как аргумент `PROGRAM()` или `DLL()`, например

```
PROGRAM(mynewprog)

OWNER(username)

SRCS(
    file_one.cpp
    file_two.cpp
)

END()
```

{% note warning %}

Менять имя статической библиотеки (`LIBRARY`) настоятельно не рекомендуется.

{% endnote %}

{% note warning %}

Имя программы/dll должно быть уникальным для всего дерева аркадии, так как по этому имени создаются символические ссылки в общую папку сборки(bin или lib)

{% endnote %}

## Q: Как мне собрать динамическую библиотеку?

**A:** Вместо `LIBRARY()` используйте [`DLL(name major_ver minor_ver)`](../manual/cpp/modules#dll), например:

```
DLL(dynalib 1 0)

OWNER(username)

SRCS(
    file_one.cpp
    file_two.cpp
)

END()
```

- Конечное имя цели как всегда зависит от платформы, например, libdynalib.so.1.0
- Для кода на Go используйте `GO_DLL`

## Q: Как мне собрать одновременно и статическую, и динамическую библиотеку?

**A:** Предположим, у вас есть библиотека в path/to/mylib, например так:

файл path/to/mylib/ya.make:

```
LIBRARY()

OWNER(username)

SRCS(
    mylib_one.cpp
    mylib_two.cpp
)

END()
```

для динамической версии создайте новый проект, например в `path/to/mylib/dynamic` и там используйте инструкцию `DLL_FOR()`, например, файл `path/to/mylib/dynamic/ya.make`:

```
DLL_FOR(path/to/mylib)
```

## Q: Как мне добавить в сборку файлы других типов (не ~C++)?

**A:** Система сборки поддерживает обработку распространённых (в аркадии) механизмов сборки, большинство файлов можно просто перечислить в `SRCS`, например:

```
PROGRAM()

OWNER(username)

SRCS(
    file_one.cpp
    file_two.c
    machine.rl6
    oldmachine.rl5
    strdata.gperf
    lexer.l
    parser.y
    factors.fml
    protobuf.proto
)

END()
```

- [Более подробно об обработке по расширеням в макросе `SRCS`](../manual/extensions.md#srcs)

## Q: У меня есть директория с файлами, я просто хочу собрать из них библиотеку не перечисляя файлы

**A:** Мы не поддерживаем glob'ы/regexp'ы для указаниях исходных файлов в общем случае. Перечисляйте файлы явно. Если список слишком длинный, можно вынести его в отдельный файл с использованием `INCLUDE`.

__project_path/files.lst__

```
SET(ALL_MY_FILES
   # длинный список файлов здесь
)
```

__project_path/ya.make__
```
LIBRARY()
    OWNER(username)
    INCLUDE(${ARCADIA_ROOT}/project_path/files.lst)
    SRCS(${ALL_MY_FILES})
END()
```

{% note info %}

У этого правила есть ряд исключений `JAVA_SRCS`. `DOCS_DIR`, `ALL_PY_SRCS`. `COLLECT_FRONTEND_FILES` и т.п., однако мы рекомендуем указывать файлы явно, чтобы быть уверенными, что локально и в автосборке вы собираете одно и то же.

{% endnote %}


## Q: Как мне добавить в мою программу данные (ресурсы, произвольные файлы)?

**A:** Используйте макрос `RESOURCE`

Включить файлы в тело программы можно в одном месте (например, в библиотеке), а использовать - в другом (например, в программе).

### Как включить содержимое файла в код:

```
LIBRARY()

OWNER(user1)

RESOURCE(
    path/to/file1 /key/in/program/1
    path/to/file2 /key2
)

END()
```

### Как получить содержимое файла по ключу:


{% list tabs %}

- C++

  __ya.make:__

  ```
  PEERDIR(
      library/cpp/resource
  )
  ```

  В source.cpp:

  ```cpp
  #include <library/cpp/resource/resource.h>

  #include <util/stream/output.h>

  int main() {
      Cout << NResource::Find("/key/in/program/1") << Endl;
      Cout << NResource::Find("/key2") << Endl;
  }
  ```

    * `/key/in/program/1` и `/key2` не должны содержать названия файлов `path/to/file1` и `path/to/file2`.

- Python:

  В питоне нужно использовать `library/python/resource`

  ```
  PEERDIR(
      library/python/resource
  )
  ```

  В source.py:

  ```py
  from library.python import resource
  r = resource.find("/key")
  ```

- Go:

  В go для доступа к ресурсам используйте модуль `a.yandex-team.ru/library/go/core/resource`

  Подгружать ресурс, когда вам нужно как-то парсить его данные можно, например, так:
  ```go
  var resourceParsedData ResourceType
  var initResourceOnce = sync.Once{}

  func GetResource() ResourceType {
  	initResourceOnce.Do(parseResource)

  	return resourceParsedData
  }

  func parseResource() {
  	resourceParsedData = ResourceType{}

  	jsonData := resource.Get("/key2")
  	if jsonData == nil {
  		return
  	}

  	_ = json.Unmarshal(jsonData, &resourceParsedData)
  }
  ```

  {% note tip %}

  Для Go поддержан встроенный механизм включения ресурсов [go:embed](../manual/go/macros.md#embed)

  {% endnote %}

{% endlist %}

{% note alert %}

Из-за особенностей инициализации ресурсов, нельзя просто так взять и создать свой собственный пакет, в котором подгружать содержимое ресурса в init'е пакета.
Так вам стабильно будет возвращаться пустые данные (будто ресурса не существует)

{% endnote %}

Если вы хотите включить содержимое ресурса сендбокса, можно использовать `FROM_SANDBOX`. Тогда в `ya.make` добавится строчка:

```
FROM_SANDBOX(1234567 OUT_NOAUTO path/to/file1) # 1234567 - номер ресурса, path/to/file1 - файл, который нужно извлечь из ресурса. Предполагается, что ресурс запакован tar
```


## Q: Как мне добавить флаги компиляции для моего проекта?

**А: Не делайте этого, они всё равно будут непереносимы.!! Некоторые частные случаи рассмотрены ниже.**


## Q: Как мне добавить определение директивы препроцессора для моего проекта?

**A:** используйте макросы `CFLAGS()`, `CONLYFLAGS()`, `CXXFLAGS()`.

__Например:__

```
LIBRARY()

OWNER(username)

SRCS(
    file_one.cpp
    file_two.cpp
)

CFLAGS(
    -DYANDEX_WEB
)

END()
```

Данные макросы определяют одноимённые переменные, которые явно задать системе сборки, чтобы воздействовать на всю сборку целиком. Например, `ya make -DCFLAGS=-DYANDEX_WEB`.
Обратите внимание на два `-D` один — это параметр ya make, второй передастся компилятору.


## Q: Как отключить оптимизацию компилятора?

**А:** Да, иногда это бывает необходимо, когда исходник столь сложен, что несовершенный оптимизирующий компилятор не может корректно его обработать. Пишите так:

```
NO_OPTIMIZE()
```

## Q: Как добавить новый макрос?

**A:** Если вам не хватает возможностей наших `ya.make` файлов обратитесть в [поддержку devtools](https://st.yandex-team.ru/createTicket?queue=DEVTOOLSSUPPORT)

## Q: Как мне собрать программу, которая включают в себя интерпретатор питона?
**А:** используйте макрос `USE_PYTHON3()` (настраивает соответствующие пути, флаги, линкуется с libpython), например:

```
DLL(searchhost)

OWNER(username)

USE_PYTHON()

PEERDIR(
    yweb/robot/hostmon/src/common
    contrib/libs/bdb
)

SRCS(
    searchhost.cpp
)

END()
```

  * `USE_PYTHON3()` не должен использоваться вместе с PYMODULE()
  * `USE_PYTHON3()` может использоваться вместе с `PROGRAM()`/`LIBRARY()`/`DLL()`

## Q: В чем разница между PYMODULE() и USE_PYTHON()?

**А:** `PYMODULE()` - нельзя использовать как библиотеку, оно может быть загружено только интерпретатором. `USE_PYTHON()` - используется для программ, которые включают в себя интерпретатор питона.

## Q: Как изменить используемый аллокатор? Какой аллокатор используется по умолчанию?

## Q: Как построить функции ввода-вывода членов перечислений, описанных в заголовке?

**A:** Используйте макрос `GENERATE_ENUM_SERIALIZATION()` или `GENERATE_ENUM_SERIALIZATION_WITH_HEADER()`, например:

```
LIBRARY()

OWNER(username)

PEERDIR(
    util/draft
    devtools/came/data
)

SRCS(
    conf.cpp
    st_conf.cpp
    out.cpp
)

GENERATE_ENUM_SERIALIZATION(unit.h)

END()
```

Если в unit.h написано

```cpp
struct TDesc {
    enum EKind {
        Source, Header, Asm
    };
    // ...
};
```

то в своём файле.cpp вы можете написать

```cpp
#include "unit.h"
#include <util/stream/output.h>
// ...
void f() {
    TDesc::EKind kind = TDesc::Asm;
    Cout << "kind is " << kind << Endl;
}
```

и получить читаемое сообщение. Сериализатор понимает перечисления в пространствах имён и структурах (классах), а также поддерживает значения, задаваемые вручную. 
`enum class` тоже поддерживаются.

Если вам нужно, чтобы ключ enum-а назывался иначе, чем его строковое значение, то можете использовать такой синтаксис:
```cpp
struct TDesc {
    enum EKind {
        K_SOURCE = 2 /* "source" */,
        K_HEADER = 3 /* "header" */,
        K_ASM    /* "asm" */
    };
};
```
В этом случае строка в двойных кавычках внутри блочного комментария будет проинтерпретирована как ключ enum-а.

{% note tip %}

Комментарий должен идти после ключа, но до запятой!

{% endnote %}

Если всё это сделать, то вам становятся доступными следующие arcadia-style функции из `util/string/cast.h`:

```cpp
template <>
void Out<TDesc::EKind>(TOutputStream& os, TDesc::EKind& n);

template <>
bool TryFromString<TDesc::EKind>(...)

template <>
TDesc::EKind FromString<TDesc::EKind>(...)

const TString& ToString(TDesc::EKind value);              // переводит значение enum-а в строку
bool TryFromString(const TStringBuf& name, TDesc::EKind& ret);  // переводит строку в значение enum-а (если не найдено, возвращает false)
```

Кроме этого, Вам доступны следующие функции из `util/generic/serialized_enum.h`:

```cpp
template<>
const TString& GetEnumAllNames<TDesc::EKind>();                // возвращает полный список значений enum-а через запятую: 'source', 'header', 'asm'

template<>
const TVector<TDesc::EKind>& GetEnumAllValues<TDesc::EKind>(); // возвращает список значений enum-а
```

Реализация лежит в [tools/enum_parser](https://a.yandex-team.ru/arc/trunk/arcadia/tools/enum_parser/).



### Известные проблемы

* enum-ы, определённые через typedef enum, не поддерживаются. 
  ```cpp
  typedef enum : uint8_t {
      ABad,
      BBad,
      CBad
  } EBadEnum;
  ```
  Ошибка выглядит так

  ```
   ------- [LD] {FAILED} $(B)/junk/zhigan/test_serialization/test_serialization{, .mf}
   command /home/zhigan/.ya/tools/v3/512070946/ymake --python /home/zhigan/tmp_arc/trunk/arcadia/build/scripts/link_exe.py /home/zhigan/tmp_arc/trunk/arcadia/devtools/gccfilter/gccfilter.pl -c /home/zhigan/.ya/tools/v3/354857069/bin/clang++ /home/zhigan/.ya/build/build_root/4xqi/000012/junk/zhigan/test_serialization/test.cpp.o /home/zhigan/.ya/build/build_root/4xqi/000012/junk/zhigan/test_serialization/test.h_serialized.cpp.o -o /home/zhigan/.ya/build/build_root/4xqi/000012/junk/zhigan/test_serialization/test_serialization -rdynamic -Wl,--start-group contrib/libs/cxxsupp/libcontrib-libs-cxxsupp.a util/libyutil.a library/lfalloc/liblibrary-lfalloc.a contrib/libs/asmlib/libcontrib-libs-asmlib.a contrib/libs/platform/tools/linkers/lld/libtools-linkers-lld.a contrib/libs/cxxsupp/libcxx/liblibs-cxxsupp-libcxx.a util/charset/libutil-charset.a contrib/libs/zlib/libcontrib-libs-zlib.a contrib/libs/double-conversion/libcontrib-libs-double-conversion.a library/malloc/api/liblibrary-malloc-api.a contrib/libs/cxxsupp/libcxxrt/liblibs-cxxsupp-libcxxrt.a contrib/libs/cppdemangle/libcontrib-libs-cppdemangle.a contrib/libs/libunwind_master/libcontrib-libs-libunwind_master.a contrib/libs/cxxsupp/builtins/liblibs-cxxsupp-builtins.a -Wl,--end-group -ldl -lrt -Wl,--no-as-needed -fuse-ld=/home/zhigan/.ya/tools/v3/360800403/ld -lrt -ldl -lpthread -nodefaultlibs -lpthread -L/usr/lib/x86_64-linux-gnu -lc -lm failed with exit code 1
   /home/zhigan/.ya/tools/v3/360800403/ld: error: duplicate symbol: unsigned char FromStringImpl<unsigned char, char>(char const*, unsigned long)
   >>> defined at test.h_serialized.cpp:128 (/home/zhigan/.ya/build/build_root/w6hf/000013/junk/zhigan/test_serialization/test.h_serialized.cpp:128)
   >>>            /home/zhigan/.ya/build/build_root/4xqi/000012/junk/zhigan/test_serialization/test.h_serialized.cpp.o:(unsigned char FromStringImpl<unsigned char, char>(char const*, unsigned long))
   >>> defined at cast.cc:612 (/home/zhigan/tmp_arc/trunk/arcadia/util/string/cast.cc:612)
   >>>            cast.cc.o:(.text+0x29E0) in archive util/libyutil.a
    ...
   ```

## Q: Как получить количество элементов в перечислении?

**A:** `GENERATE_ENUM_SERIALIZATION_WITH_HEADER()`

Этот макрос, если его использовать *вместо* предыдущего, генерирует для файла, например, `myenum.h` дополнительно .h-файл с именем `myenum.h_serialized.h` с интерфейсом, объявленным в 
`util/generic/serialized_enum.h`:

Пока тут только одна функция:

```cpp
template <typename EnumT>
constexpr size_t GetEnumItemsCount();
```

Т.е., если у вас есть такой файл с enum-ом enum.h:

```cpp
enum class EGenders {
  Male,
  Female
}
```

то этот макрос сгенерирует следующий файл (специализацию шаблонов из serialized_enum.h):

```cpp
template <>
constexpr size_t GetEnumItemsCount<EGenders>() {
    return 2;
}
```

Что позволит вам получать количество значений в enum'е простым вызовом `GetEnumItemsCount<EGenders>()`, в том числе в compile-time, 
например: `static_assert(GetEnumItemsCount<EGenders>() == 2, "something strange");`

Q: Как получить версию программы/флаги сборки/версию репозитория?

**A:** Версия вшивается во все программы и получить её можно с помощью

- [libarary/cpp/svnversion](https://a.yandex-team.ru/arc_vcs/library/cpp/svnversion)
- [libarary/python/svn_version](https://a.yandex-team.ru/arc_vcs/library/python/svn_version)
- [libarary/java/svnversion](https://a.yandex-team.ru/arc_vcs/library/java/svnversion)
- [library/go/core/buildinfo](https://a.yandex-team.ru/arc_vcs/library/go/core/buildinfo)

Версия доступна и для svn и для arc  несмотря на названия библиотек.


__Пример:__

```
PROGRAM()

OWNER(username)

SRCS(
    main.cpp
    hist.cpp
)

PEERDIR(
    quality/mapreducelib
    quality/mr_util
    library/cpp/svnversion
    yweb/antispam/pfc
)

END()
```

## Q: Как добавить unittest?

**A:** В поддиректории ut/ необходимо написать `ya.make` используя макросы `UNITTEST() ... END()`:
Название unittest-а должно быть **уникальным**.

Рекомендуется задавать название unittest-а, как название проекта (для которого написан unittest) + суффикс `_ut`

```
UNITTEST()

OWNER(username)

PEERDIR(
    ADDINCL tools/communism/lib
)

SRCS(
    request_ut.cpp
    solver_ut.cpp
)

END()
```

Так же для замены цепочки из UNITTEST()/PEERDIR(ADDINCL)/SRCDIR() существует такой вариант:

```
UNITTEST_FOR(tools/communism/lib)

OWNER(username)

SRCS(
    request_ut.cpp
    solver_ut.cpp
)

END()
```

+ надо добавить `UNITTEST` в автосборку. Для этого надо поставить `RECURSE` в родительской директории.
