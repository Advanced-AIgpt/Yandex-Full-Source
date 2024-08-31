# Cowsay - пример внешнего скилла для Алисы на Java
Скилл умеет на вопрос "Как говорит коровка?" отвечать "Муу!".
Разбор входной фразы сделан при помощи технологии Granet-ов, поэтому можно спрашивать
"Что сказала корова" например и т.п.

## Сборка
```
[cowsay]$ ya make --checkout
```

## Запуск
```
[cowsay]$ ./run.sh ru.yandex.alice.megamind.example.Application
```

## Интеграция с megamind
Для того чтобы запустить скилл в девелоперском режиме, с локально поднятым Megamind-ом нужно:

* Добавить конфиг сценария (в данном случае cowsay.pb.txt) в Мегамайнд в соотвествии с [инструкцией](https://wiki.yandex-team.ru/alice/megamind/protocolscenarios/scenariosconfig/#dobavlenijakonfigavmegamind).

* Запустить Megamind

* Запустить сценарий

* Собрать утилиту alice/nlu/granet/tools/granet/granet

* Сконвертить гранет сценария в готовый флаг эксперимента. Команда примерно такая
```
  [arcadia]$ alice/nlu/granet/tools/granet/granet grammar pack -g "alice/megamind/example_apps/java/cowsay.grnt" --lang ru
```
На выходе утилиты будет длинная строчка в base64 кодировке. Нужно ее передавать в запрос с флагом эксперимента:
```
bg_granet_source_text=<long_base64_string>
```

См. также скрипт https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/test/granet/print_grammar_as_experiment.sh
Но этот скрипт конвертит main.grnt, главный гранет, в который импортятся все гранеты всех сценариев. Если им ползоваться, то
нужно предварительно заимпортить в main.grnt свой гранет.

* Флаг эксперимента передается с запросом. Способ зависит от клиента. Например, в случае vins_client это флаг `-e`


## Неочевидные моменты

### Момент 1
Spring из коробки поддерживает protobuf, однако его коробочный ProtobufJsonFormatHttpMessageConverter не работает с
Megamind, потому что ожидает HTTP заголовок "ContentType: application/x-protobuf". Megamind шлет "ContentType: application/protobuf".
Поэтому в проекте есть кастомный CustomProtobufHttpMessageConverter конвертер.

### Момент 2
Сценарий в своем конфиге содержит раздел AcceptedFrames, где перечислены имена интентов, которые он поддерживает. Megamind
в общем случае фильтрует фреймы, чтобы в сценарий не прилетали заведомо неподдерживаемые запросы. Однако это не всегда работает,
например это не работает в кейсе, когда сценарий - это "активный" сценарий (активным считается последний использованный).
Поэтому в самом сценарии нужно впиливать проверку, что в реквесте есть фрейм, который он поддерживает. Если нет, то
возвращать is_irrelevant.