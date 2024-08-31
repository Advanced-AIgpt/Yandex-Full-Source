# Пакетирование и объединение : модули

Модули, используемые для объединения сборочных целей

- [UNION](#union)
- [PACKAGE](#package)

Модули, используемые для пакетирования и объединения, имеют два свойства, отличающих их от модулей для сборки определённого языка:

1. Оба эти модуля не поддерживают [обработку файлов по расширениям](../extensions#srcs). Файл, добавленный в сборку модуля любым макросом
   [`RUN_PROGRAM`](../common/macros#run_program), [`FROM_SANDBOX`](../common/data#from_sandbox), [`COPY_FILE`](../common/macros#copy_file)
   даже при указании `OUT` и `AUTO` не будет обработан.

2. Любой файл, добавленный в сборку модуля, не будет обработан модульной командой, а станет непосредственным результатом сборки самого модуля.
   Расположение этого результата, однако, зависит от типа модуля.


## UNION

`UNION`

Описывает группу сборочных целей, включая как модули, таки  файлы. 

`UNION` — это как библиотека для [`PACKAGE`](#package), он не собирает свои [`PEERDIR`](../common/macros#peerdir)-зависимости, они предоставляются [`PACKAGE`](#package), который играет роль программы
и замыкает все свои `UNION`-ы. Как и библиотеки, `UNION` собирает свои собственные *файлы*. При сборке `UNION` макросы, написанные в нём, будут выполнены и
если они описывают сборку, то результирующие файлы станут результатами этого `UNION`. При этом они будут сложены непосредственно в директорию этого `UNION`.

Часто `UNION` содержит такие макросы как 

- [`RUN_PROGRAM`](../common/macros#run_program) и подобные - генерация файлов программой или скриптом 
- [`FROM_SANDBOX`](../common/data#from_sandbox) - загрузка файлов из Sandbox
- [`BUNDLE`](../common/macros#bundle) - получение файла-результата сборки другого модуля
- [`FILES`](./macros#files)/[`COPY_FILE`](../common/macros#copy_file) - получение файлов из репозитория

{% note tip %}

При обычной сборке, когда результаты оказываются символьными ссылками в репозитории макрос `FILES` как будто бы не имеет эффекта: его результаты оказались бы на месте существующих файлов,
и потому при создании символьных ссылок они игнорируются. Однако, макрос всегда обрабатывается и имеет два видимых эффекта:

1. При сборке в отдельную директорию с параметром [`-o`/`--output`](../../usage/ya_make/#results) файлы будут скопированы в выходную директорию, как и все остальные результаты модуля.
2. При [*резолвинге*](../../general/how_it_works.md#resolving) входных файлов такой файл может быть выбран, если он будет указан явно (как `${ARCADIA_BUILD_ROOT}/path/to/the.file`) или
   окажется наиболее приоритетным из доступных (например, при [*резолвинге*](../../general/how_it_works.md#resolving) инклудов, где сборочная директория имеет приоритет над репозиторием).

{% endnote %}

На данный момент [`PEERDIR`](../common/macros#peerdir) на UNION можно поставить только из [`PACKAGE`](#package) или другого `UNION`. Вопрос о том, чтобы разрешить [`PEERDIR`](../common/macros#peerdir)
на `UNION` из программ и библиотек как на модуль генерирующий (но не собирающий) исходный код сейчас прорабатывается.


**Пример:**

Построим `devtools/examples/package/data`

**devtools/examples/package/data/ya.make**

```(yamake)
OWNER(g:ymake g:yatool)

UNION()

FILES(
    data.txt
)

PEERDIR(
    devtools/examples/package/union2
)

END()
```

Строим в отдельную директорию, чтобы увидеть эффект макроса [`FILES`](./macros#files) 

```
[=] ya make devtools/examples/package/data -o ~/res
Ok
```

Единственный файл в `~res` — это `devtools/examples/package/data/data.txt`.

{% note tip %}

В данном случае путь от корня Аркадии — это не *namespace* как в пакете, а просто результат раскладки результатов во внешнюю директорию.
Легко проверить, что если бы мы собирали аналогичным образом пакет (например, `devtools/examples/package/main`), то весь его результат,
начиная от `devtools` оказался бы внутри `~/res/devtools/examples/package/main`.

```
[=] ./ya make devtools/examples/package/main -o ~/res
Ok
[=] ls ~/res/devtools/examples/package/main
devtools
```

{% endnote %}


**Пример2:**

Соберём `devtools/examples/package/union2`.

**devtools/examples/package/union2/ya.make**

```(yamake)
OWNER(g:ymake g:yatool)

UNION()

BUNDLE(devtools/dummy_arcadia/cat)

PEERDIR(devtools/dummy_arcadia/hello_world)

END()
```

Соберём опять во внешнюю директорию

```
[=] rm -rf ~/res
[=] ya make devtools/examples/package/union2 -o ~/res
Ok
```
Единственный файл в `~res` — это `devtools/examples/package/union2/cat`. Макрос [`BUNDLE`](../common/macros#bundle) сделал его непосредственным результатом `UNION`,
программа была собрана и сложена в директорию модуля. При этом [`PEERDIR`](../common/macros#peerdir) не был обработан. Но он будет учтён при сборке `UNION`
в `PACKAGE`, что можно было видеть в [соответствующем примере](#res_pkg).

### Использование UNION в тестах

Кроме [`PEERDIR`](../common/macros#peerdir) из модуля [`PACKAGE`](#package) все `UNION` замыкаются макросом [`DEPENDS`](../tests/common#depends) в тестах. Это позволяет
использовать `UNION` для [объединения различных данные для тестов](../common/data#union) и переиспользования этого набора между разными тестами.

`UNION` будет собран так, словно модуль теста — это [`PACKAGE`](#package). В сборочной директории теста `yatest.common.build_path` и аналогичных окажется `UNION` по его пути от корня Аркадии.
Внутри будут его собственные файлы, а в своих *namespace* (путях от корня Аркадии) окажутся его зависимости.



## PACKAGE

`PACKAGE`

Описывает сборку нескольких сборочных целей в один *пакет*.

**Пакет** представляет собой структуру директорий на файловой системе или в архиве, где каждая зависимость модуля `PACKAGE` присутствует со своим *namespace* - путём от корня Аркадии.
Такая раскладка позволяет избежать коллизий, если цели у разных зависимостей называются одинаково.

В пакет складываются:

- Все результаты всех сборочных модулей, на которые у `PACKAGE` есть [`PEERDIR`](../common/macros#peerdir). `PACKAGE` транзитивно замыкает все `UNION` по [`PEERDIR`](../common/macros#peerdir).
- Все результаты всех генерирующих макросов, написанных в самом `PACKAGE`.

Всё это становится результатами модуля `PACKAGE` и это надо учитывать при [`PEERDIR`](../common/macros#peerdir) на него с другого `PACKAGE`.

{% note info %}

В случае [`PEERDIR`](../common/macros#peerdir) из одного `PACKAGE` на другой вложенных *namespace* не образуется, т.е. в таком [`PEERDIR`](../common/macros#peerdir) зависимый `PACKAGE`
ведёт себя так же, как `UNION`. Всё его содержимое оказывается разложенным также, как если бы оно было содержимым зависящего `PACKAGE`, т.е. по собственным *namespace* без учёта зависимого `PACKAGE`.

{% endnote %}

`PACKAGE` является финальной целью и [`PEERDIR`](../common/macros#peerdir) на него можно ставить только из другого `PACKAGE`.  Однако на него можно ставить 
[`DEPENDS`](../tests/common#depends) чтобы получить данные в тестах. Важно только помнить о структуре результата с *namespace*-ами. Указывать `PACKAGE` в макросе
[`BUNDLE`](../common/macros#bundle) не имеет смысла, поскольку этот макрос привозит единственный артефакт модуля и директорию пакета привезти не сможет.

**Пример [`devtools/examples/package`](https://a.yandex-team.ru/devtools/examples/package):**


```
devtools/examples/package
├── data
│   ├── data.txt
│   └── ya.make
├── main
│   └── ya.make
├── sub
│   ├── data_sub.txt
│   └── ya.make  
├── union
│   └── ya.make
└── union2
    └── ya.make
```

**devtools/examples/package/sub/ya.make**

```(yamake)
OWNER(g:ymake g:yatool)

PACKAGE()

FILES(
    data_sub.txt
)

PEERDIR(
    devtools/examples/package/union
    devtools/examples/package/data
    devtools/dummy_arcadia/cat
)

END()
```

**devtools/examples/package/union/ya.make**

```(yamake)
OWNER(g:ymake g:yatool)

UNION()

PEERDIR(
    devtools/examples/package/data
    devtools/examples/package/union2
)

END()
```


**devtools/examples/package/data/ya.make**

```(yamake)
OWNER(g:ymake g:yatool)

UNION()

FILES(
    data.txt
)

PEERDIR(
    devtools/examples/package/union2
)

END()
```

**devtools/examples/package/union2/ya.make**

```(yamake)
OWNER(g:ymake g:yatool)

UNION()

BUNDLE(devtools/dummy_arcadia/cat)

PEERDIR(devtools/dummy_arcadia/hello_world)

END()
```

Собираем `devtools/examples/package/sub`:

```
[=] ya make devtools/examples/package/sub
ok

[=] ls -la devtools/examples/package/sub
total 1
drwxrwxr-x 1 user group   0 May  5 12:59 .
drwxrwxr-x 1 user group   0 May  5 12:51 ..
-rw-rw-r-- 1 user group   0 May  5 11:56 data_sub.txt
lrwxrwxr-x 1 user group  71 May  5 12:59 devtools -> /home/user/.ya/build/symres/c2010a1094de711018b25ff0a4160ba9/devtools
-rw-rw-r-- 1 user group 183 May  5 12:58 ya.make

[=] ls devtools/examples/package/sub/devtools
dummy_arcadia  examples
```

{ #res_pkg2 }
Таким образом в `devtools/examples/package/sub` образовалась структура директорий пакета. Поскольку все файлы пакета лежат внутри директории `devtools`, то на верхнем уровне она одна. Посмотрим, что внутри:

```
devtools/examples/package/sub/devtools
├── dummy_arcadia
│   ├── cat
│   │   └── cat                 # Программа из PEERDIR в .../sub/ya.make
│   └── hello_world
│       └── hello_world         # Программа из транзитивного PEERDIR в .../union2/ya.make
└── examples
    └── package
        ├── data
        │   └── data.txt        # Из соответствующих ya.make
        ├── sub                 # При чём собственные файлы из ya.make PACKAGE
        │   └── data_sub.txt    # и зависимого UNION разложены одинаково по их namespace
        └── union2       
            └── cat             # BUNDLE делает артефакт результатом модуля, в котором написан
```

Усложним пример - построим `devtools/examples/package/main`, в котором есть [`PEERDIR`](../common/macros#peerdir) на `.../sub`.

**devtools/examples/package/main/ya.make**

```(yamake)
OWNER(g:ymake g:yatool)

PACKAGE()

PEERDIR(
    devtools/examples/package/sub
    devtools/examples/package/union
)

END()
```

Строим:

```
[=] ya make devtools/examples/package/main
ok

[=] ls -la devtools/examples/package/main
total 1
drwxrwxr-x 1 user group   0 May  5 16:25 .
drwxrwxr-x 1 user group   0 May  5 12:51 ..
lrwxrwxr-x 1 user group  71 May  5 16:25 devtools -> /home/user/.ya/build/symres/abaa025d6f782c5ada013cc9f79faf95/devtools
-rw-rw-r-- 1 user group 124 May  5 16:25 ya.make

[=] ls devtools/examples/package/main/devtools
dummy_arcadia  examples
```

Структура пакета, образовавшаяся в `devtools/examples/package/main` полностью повторяет предыдущую: собственных файлов в новом пакете не было,
а файлы из зависимостей размещены в соответствии с их путями от корня Аркадии и транзитивный [`PEERDIR`](../common/macros#peerdir) через другой `PACKAGE` (`.../sub`) ничего не изменил.
