# Правила разработки в Аркадии на Go

## Полезные ссылки
* [Клуб в этушке](https://clubs.at.yandex-team.ru/golang/) - основные новости Go в Аркадии
* [Настройка](https://docs.yandex-team.ru/arcadia-golang/getting-started)  локального окружения
* [Очередь](https://st.yandex-team.ru/DEVTOOLS/order:updated:true/filter?resolution=empty()&tags=go) с фичами и багами на поддержку Go в Аркадии.
* [Awesome Go](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/README.md)
* [Список](https://a.yandex-team.ru/arc/trunk/arcadia/build/rules/go/vendor.policy) разрешенных к использованию контрибов
* [Очередь](https://st.yandex-team.ru/filters/filter:38070) на добавление Go контрибов в Аркадию и [форма](https://st.yandex-team.ru/createTicket?queue=CONTRIB&_form=12959) для заведения тасков.
* [Утилита](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/yo) для **генерации и обновления ya.make-ов для go проектов**, а так же добавления и обновления контрибов, [подробное описание](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/yo/README.md)
* Аркадийный [godoc](https://godoc.yandex-team.ru/pkg/a.yandex-team.ru/)
* [Работа с контрибами](https://docs.yandex-team.ru/arcadia-golang/contrib)
* [Tutorials](https://docs.yandex-team.ru/arcadia-golang/tutorials)
* [Arcadia Go Tutorial](https://docs.yandex-team.ru/ya-make/tutorials/go) из официальной документации по ya.make
* [Форматирование](https://docs.yandex-team.ru/arcadia-golang/formatting/) Go кода
* [Линтинг](https://docs.yandex-team.ru/arcadia-golang/linting) Go кода

Если у вас есть желание непоправимо улучшить жизнь гоферов в Аркадии, то вы можете взять какую-нибудь задачу [отсюда](https://st.yandex-team.ru/IGNIETFERRO/order:updated:false/filter?tags=go&resolution=empty()) или написать нам на рассылку [go-com@yandex-team.ru](mailto:go-com@yandex-team.ru).

## Репозиторий, базовые настройки и правила
1. Аркадия использует модули и может чекаутиться куда угодно. Чекаутится директория `trunk/arcadia` из Аркадии.
2. Аркадийный Go тулчейн прописывается в GOROOT - `export GOROOT=$(ya tool go --print-toolchain-path)`.
3. Наш Go код в Аркадии может класться куда угодно внутри `trunk/arcadia` кроме директории `vendor` и других "зарезервированых" директорий (например, `contrib`).
4. Для импортов нашего кода используется "префикс" `a.yandex-team.ru`. Например, пакет лежащий в `trunk/arcadia/myproject/mypackage`, импортируется при помощи `import "a.yandex-team.ru/myproject/mypackage"`.
5. Все контрибы кладутся в `trunk/arcadia/vendor` согласно стандартным правилам Go. Например, контриб `github.com/pkg/errors` кладётся в `trunk/arcadia/vendor/github.com/pkg/errors`.

## Лейаут проектов написаных на Go
У нас нет жестких правил для лейаута проектов. Однако для удобства совместной работы мы рекомендуем использовать несколько общих правил, основаных на [некотором](https://medium.com/golang-learn/go-project-layout-e5213cdcfaa2) [опыте](https://medium.com/@benbjohnson/standard-package-layout-7cdbc8391fc1) [комьюнити](https://github.com/golang-standards/project-layout).

Разбивайте код на 3 основных директории - `cmd`, `pkg` и `internal`.
1. `cmd` - директория для бинарников (main-пакетов). Каждый main пакет кладите в отдельную директорию с таким же названием как и результирующий бинарник. Это позволяет явно отделить бинарники от всего остального и поддерживает несколько бинарников в проекте. Делайте так даже если у вас один бинарник, а не несколько. Исключение - когда кроме main пакета больше ничего нет, в таком случае кладите всё сразу в корень проекта.
2. `pkg` - директория для публичных пакетов. Подразумевается, что данные пакеты имеет смысл импортировать другими проектами. Если у вас нет `main` пакета (вы делаете библиотеку) и у вас нет `internal` пакетов (см. ниже), или же публичных пакетов не много, то можно сложить всё в корень проекта. Use common sense.
3. `internal` - директория для пакетов, которые нельзя импортировать откуда-то кроме как из пакетов, у которых хотя бы одна родительская директория является непосредственной родительской директорией пакета `internal`. Сюда стоит класть всё то, что является деталями реализации и не предполагает импорта сторонними пакетами. Более подробно можно почитать [тут](https://golang.org/cmd/go/#hdr-Internal_Directories).

Пример лейаута проекта использующего все директории:
```
a.yandex-team.ru/myproject/cmd/myserver/main.go
a.yandex-team.ru/myproject/cmd/mytool/main.go
a.yandex-team.ru/myproject/pkg/myserverapi/myserverapi.go
a.yandex-team.ru/myproject/pkg/mylibrary/mylibrary.go
a.yandex-team.ru/myproject/internal/somethingonlymyprojectcanuse.go
a.yandex-team.ru/myproject/internal/mylib/andthistoo.go
```

Примеры реальных проектов:
* [yo](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/yo)

## Дебагер
`ya tool dlv`

## Protobuf и gRPC
Генерация протобафов для `go` требует соблюдения определённых правил:
1. Во все `.proto` файлы нужно добавить `option go_package` c полным путём импорта включая префикс `a.yandex-team.ru`. [Пример](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/dummy_arcadia/go/proto/tutorial/addressbook.proto?rev=4316509#L3).
2. Все `.proto` файлы одного "пакета" (имеющие идентичный `go_package`) должны быть "использованы" в одном и том же `PROTO_LIBRARY`. Пересечения "пакетов" в одной `PROTO_LIBRARY` недопустимы.
3. В `ya.make` с `PROTO_LIBRARY` нужно включить генерацию для `go` добавив макрос `INCLUDE_TAGS(GO_PROTO)`.

Сгенерированый "пакет" можно переименовать добавив `;name` в конце `go_package`. Это актуально для старых спек, которые очень сложно привести в "адекватное" состояние. Например, для `option go_package = "a.yandex-team.ru/bestproject/proto";` пакет будет называться `proto`, что **совсем** не ок. Самое правильное - переместить генерацию в директорию, которая будет отображать реальный смысл спеки (например, сервис для которого эта спека написана). Если же это невозможно, то нужно сделать переименование, например `option go_package = "a.yandex-team.ru/bestproject/proto;bestprojectpb";`.

Помните, что последнее "слово" в `go_package` будет названием пакета, по которому пользователи будут обращаться к сгенерированому коду. Делайте его адекватным и (крайне желательно) идентичным названию директории, в которую будет генерироваться "пакет". Иначе ваши пользователи будут импортировать `a.yandex-team.ru/foo`, а использовать сгенерированый код как `bar.Something`.

### Что делать с PROTO_LIBRARY, где .proto файлы разных пакетов лежат в одной директории
С legacy протобуф библиотеками часто возникает пролема с разположением файлов. Если все .proto файлы собираются одним ya.make файлом, то из них получится один go пакет. При этом, логически .proto файлы могут описывать несколько пакетов.

Правильный вариант исправления - разложить .proto в соответствии их структурой пакетов (как написано выше).

Если это не представляется возможным, то есть второй вариант - можно написать _параллельное дерево ya.make-ов_. [Пример](https://a.yandex-team.ru/arc/trunk/arcadia/yt/go/proto). Очевидный минус такого решения - нужно поддерживать два дерева ya.make-ов для одних и тех же протобафов.

## FAQ
**Q**: `go get` не работает на Аркадию. Как мне использовать Аркадийный гошный код вне Аркадии?

**A**: Из коробки никак и в будущем не планируется. Более того, это невозможно поддержать для кода использующего cgo. Наша рекомендация - переезжайте в Аркадию. Если очень надо, то как временную меру можно использовать селективный чекаут `svn` в ваш `vendor` и/или `replace` в вашем `go.mod`.

-----

**Q**: Как мне собрать статический бинарь?

**A**: `ya make -DCGO_ENABLED=0`. Если статический бинарь используется в проде, его можно добавить в [отдельную платформу](https://a.yandex-team.ru/arc/trunk/arcadia/autocheck/linux/cgo_targets.inc) автосборки.

-----

**Q**: Как мне использовать билд теги?

**A**: Аркадийная сборка не смотрит в билд теги go файлов, поэтому если вы укажите билд тег в исходном коде - никакого эффекта от него не будет. В ya make есть переменные, которые имеют примерно тот же смысл, что и билд теги.

Билд теги linux, darwin и windows автоматически распознаются в yo fix. Например, вот так выглядит сборка платформозависимого кода.

```
SRCS(user.go)

IF (OS_LINUX)
SRCS(user_linux.go)
ENDIF()

IF (OS_WINDOWS)
SRCS(user_windows.go)
ENDIF()
```

Для разделения тестов на быстрые и медленные нужно использовать макрос SIZE в ya.make.

Кастомные билд теги не поддерживаются в ya make.

-----

**Q**: Почему при сборке тестов возникает ошибка `internal compiler error: conflicting package heights...` или `fingerprint mismatch: <path> has <hash>, import from <another path> expecting <another hash>`?

**A**: ya make не поддерживает сборку тестов пакета A, в которых есть псевдо-импорт-цикл A_test -> B -> A. Такая последовательность импортов не является импорт-циклом с точки зрения го, но не укладывается в сборочный граф ya make. https://st.yandex-team.ru/DEVTOOLS-5045

**Workaround**: Унести проблемный тест в другой пакет.

{% cut Пример %}

```
a.yandex-team.ru/library/go/core/log/log_test.go:10:2: internal compiler error: conflicting package heights 12 and 9 for path "a.yandex-team.ru/library/go/core/log"
```
Тут `log_test.go` создает проблемы, из-за того что импортирует пакет `log/zap`, который потом импортирует `log`.
Решение - унести `log_test.go` в `library/go/core/log/test/log_test.go`

{% endcut %}

-----

**Q**: У меня не работает запуск дебагера в IDE. Говорит, не хватает протобуфов. Что делать?

**A**: При сборке в аркадии, .go файлы протобуфов не комитятся в репозиторий. Их генерирует ya make на этапе сборки, и по дефолту кладёт в build-директорию. Можно попросить ya make поставить симлинки из source-директории, запустив команду `ya make my/project --replace-result --add-result .go`.

-----

**Q**: Как мне запустить тесты с race детектором?

**A**: Race detector поддерживается в автосборке как [отдельная платформа](https://a.yandex-team.ru/arc/trunk/arcadia/autocheck/linux/go_race_targets.inc) и локально через `ya make -t --race`.

-----

**Q**: Можно ли использовать GOPATH?

**A**: Можно, но не рекомендуется.

-----

**Q**: Можно ли делать модули в своих проектах?

**A**: Нет. Аркадийная сборка ничего не знает о модулях. Если вы добавите go.mod в свой проект, то он будет игнорироваться, но при этом вы вероятно сломаете поддержку IDE для остальных пользователей Аркадии (в своём проекте).

-----

**Q**: У меня странные пути к сорцам при отладке в Delve, которые начинаются с `/-S/`. Что делать?

**A**: В Аркадии включена подмена build/source рутов для унификации сборочной машинерии. Для починки предлагается настроить `substitute-path` для dlv:

```
$ cat ~/.config/dlv/config.yml
substitute-path:
- {from: /-S/, to: /path/to/arcadia/root/}
- {from: /-B/, to: /path/to/arcadia/root/}
```

Подробнее про конфигурирование delve можно прочитать в [в офф докуменатции](https://github.com/go-delve/delve/blob/master/Documentation/cli/README.md).
