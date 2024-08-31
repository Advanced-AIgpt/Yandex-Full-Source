# Roadmap

На данной странице Go-комитет отражает текущее реализованное и будущее желаемое состояние поддержки Go в Аркадии.

## Что уже есть

### Layout
Мы определили базовый layout Go в Аркадии, который опирается на введенную в Go 1.5 концепцию явного вендоринга зависимостей. С развитием будущих версий языка и стабилизации концепции модулей мы планируем рассмотреть возможность перехода на модульный layout Go в Аркадии.

### Контрибы
Уже сейчас определены: политика выбора контрибов, порядок добавления и инструменты для работы с контрибами в Аркадии.

### Тулинг
Мы старались сделать так, чтобы стандартный и сторонний тулинг работали максимально нативно. На данный момент, при соблюдении правил [чекаута Аркадии](https://wiki.yandex-team.ru/arcadia/gocommittee/GoDevRules/#repozitorijjbazovyenastrojjkiipravila),
любая популярная IDE, линтер или другой инструмент, совместимый со стандартным layout'ом `GOPATH`, должен работать без каких-либо проблем.

### Сборка и тесты через ya make
На данный момент сборка и тестирование относительно простых проектов на Go с помощью команд `ya make` и `ya make -t` должны работать без каких-либо проблем.

### Protobuf и GRPC кодогенерация
В данный момент реализована поддержка [базовой кодогенерации](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/dummy_arcadia/go/grpc) `protobuf` и `grpc` для Go проектов.

### Локальный линтинг
Комитетом был выбран тул для линтинга и определены обязательные и рекомендуемые правила линтинга Go кода в Аркадии. На данный момент поддерживается только локальный линтинг кода на рабочем компьютере разработчика.

### Линтинг в CI
Мы хотим, чтобы обязательные правила линтинга стали частью политики [Зеленого Транка](https://wiki.yandex-team.ru/users/stanly/vcs/green-trunk).
Поэтому в будущем к локальному линтингу добавится линтинг на уровне Continuous Integration, который в большинстве случаев не позволит нежелательному коду попасть в основную ветку Аркадии.

## Дальнейшие работы
С помощью команды DevTools в ближайшем будущем мы планируем внедрить в Аркадию следующие пункты:

### Дополнительные опции сборки
Планируется поддержать дополнительные опции при сборке и тестировании проектов: `-race`, `-ldflags`, `-tags` и другие.

### Покрытие тестами и бенчмарки
Одним из пунктов, которые обязательно будут поддержаны в Аркадии, является учет покрытия Go кода тестами. Также в планах поддержка в Аркадии замеров производительности кода.

### Расширенная кодогенерация
В будущем планируется поддержка дополнительных инструментов кодогенерации: [go-swagger](https://github.com/go-swagger/go-swagger), [messagepack](https://github.com/tinylib/msgp) и прочих.

{% note info %}

Стоит отметить, что в настоящий момент у комитета и команды DevTools нет точных сроков и порядка реализации представленных выше улучшений поддержки Go в Аркадии.

{% endnote %}

