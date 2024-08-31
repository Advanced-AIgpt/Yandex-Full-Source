### Тестирование грамматики в Алисе

{% note info %}

Результатом описанной ниже процедуры будет появление вашего интента в
разборах Бегемота. Но для того, чтобы Мегамайнд начал ходить в ваш
сценарий, требуется также [настройка самого Мегамайнда](../../megamind/config.md).

{% endnote %}

Мегамайнд получает разборы Гранета из Бегемота. Чтобы Бегемот начал разбирать ваш интент, можно:
- Закомитить грамматику и дождаться релиза Бегемота ([это долго](https://wiki.yandex-team.ru/alice/begemot/release/#orelize)).
- Развернуть свой Бегемот ([это муторно](https://wiki.yandex-team.ru/alice/begemot/beta/)).
- Выкатить изменения через быстрые данные Бегемота (подробности ниже).
- Если вы не хотите, чтобы из-за комита ваша грамматика случайно попала в прод, её можно спрятать под эксперимент. Для этого добавляем имени формы (или сущности) суффикс с названием эксперимента: `alice.random_number.ifexp.bg_enable_my_new_random_number`. Далее выполняем первый пункт, но дополнительно передаём флаг `bg_enable_my_new_random_number` (подробности ниже).
- Запаковать вашу грамматику в base64 и передавать её через флаг эксперимента (подробности ниже).

Для отладки можно послать запросы напрямую в Бегемот, в обход Мегамайнда. Сделать это можно с помощью [этой формочки](https://wiki.yandex-team.ru/users/samoylovboris/begemot/request/). Флаги экспериментов, начинающиеся с `bg_`, надо писать через точку с запятой в поле `wizextra`. В ответе нужно искать ключ "Granet".

### Быстрые данные {#fresh}

Гранету в Бегемоте доступны две версии одного и того же набора грамматик:
- Статические грамматики. Релизятся [раз в неделю](https://wiki.yandex-team.ru/alice/begemot/release/#orelize) вместе с релизом Бегемота.
- Свежие грамматики (быстрые данные). Берутся из Аркади каждые 20 минут [этой sandbox-таской](https://sandbox.yandex-team.ru/scheduler/16328/view).

По умолчанию Гранет использует только статические грамматики. Включить грамматики из быстрых данных можно двумя способами:
с помощью флагов эксперимента Мегамайнда, либо с помощью параметров `fresh` или `freshness`.

#### Для быстрого тестирования

Чтобы Гранет использовал свежие грамматики вместо статических, можно передать в Мегамайнд один из следующих экспериментов:
- `bg_fresh_alice` - использовать свежие грамматики.
- `bg_fresh_alice_form=my_form` - использовать свежие грамматики для формы `my_form`.
- `bg_fresh_alice_entity=my_entity` - использовать свежие грамматики для сущности `my_entity`.
- `bg_fresh_alice_prefix=my_prefix` - использовать свежие грамматики для форм и сущностей, имена которых начинаются с `my_prefix`.

Если нужно использовать быстрые данные для нескольких форм, нужно передать несколько экспериментов: `bg_fresh_alice_form=my_form_1`, `bg_fresh_alice_form=my_form_2` (каждая строчка это имя одного эксперимента, а не имя плюс значение). Либо передать `bg_fresh_alice_prefix=my_form_`.

Этот подход используется для тестирования изменений. Допустимо запускать AB эксперименты на небольшой процент по согласованию с аналитиками с флагами свежих грамматик, но не с `bg_fresh_alice`, только с конкретными свежими формами.

#### Для быстрого релиза

Быстро зарелизить в прод изменения в гранетной форме или сущности, можно двумя способами:
- Выставить у формы или сущности параметр `fresh: true`. Тогда любые изменения в этой форме или сущности будут сразу выезжать в прод (в течение получаса после коммита). [Пример](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/granet/demo/demo.grnt?rev=7455865#L42).
- Если необходимо разовое обновление грамматики, нужно добавить параметр `freshness` и увеличить его на единицу. Это более безопасный подход, чем `fresh: true`, так как последующие изменения в той же грамматике не будут автоматически выкатываться в прод. [Пример](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/granet/demo/demo.grnt?rev=7455865#L49).

Параметры `fresh` и `freshness` можно выставлять только у следующих форм и сущностей:
- Любые формы и сущности для колдунщиков ([arcadia/alice/nlu/data/wizard_ru/granet](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/wizard_ru/granet))
- Любые формы и сущности для внешних навыков ([arcadia/alice/nlu/data/paskills_ru/granet](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/paskills_ru/granet)).
- Голосовые действия - формы с флагом `is_action`. Такие формы обрабатываются, только если активный сценарий запросил их через списов экшенов ([TScenarioResponseBody::FrameAction](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/scenarios/response.proto?rev=7176144#L253)).
- Формы с флагом `is_fixlist`. Такие формы должны матчиться не более чем на 1/10000 потока. Это проверяется обязательными для таких форм medium-тестами.
- Сущности, которые используются только в вышеперечисленных формах.

Пример:

```bash
form alice.my_scenario.turn_on:
    is_action: true
    freshness: 1
    root: включи
```

{% cut "Почему так сделано? Почему бы не использовать только свежие грамматики?" %}

Дело в том, что в грамматиках можно сделать ошибку, которая перетянет в интент много реплик из других интентов. Неосторожный комит в грамматики может сильно ухудшить качество Алисы. При релизе Бегемота запускается множество тестов, в частности тяжеловесный, полуручной тест UE2E Алисы. Эти тесты не получится запускать на каждое обновление свежих грамматик. Поэтому на релиз грамматик через быстрые данные накладываются дополнительные ограничения, описанные выше.

{% endcut %}

### Как спрятать свои изменения под флаг эксперимента {#experiment}

Эти изменения записываются в виде "экспериментальной" формы или сущности.

Форма считается "экспериментальной", если её имя имеет вид `имя_формы.ifexp.имя_эксперимента`. Бегемотный Гранет работает ней по-особенному:
- без флага `имя_эксперимента`, такая форма будет проигнорирована.
- под флагом `имя_эксперимента`, форма `имя_формы.ifexp.имя_эксперимента` подменит собой форму `имя_формы`. Её результаты будут видны под именем `имя_формы` (а не `имя_формы.ifexp.имя_эксперимента`).

В тестах Гранета "экспериментальные" формы существуют на тех же правах, что и обычные. То есть одновременно могут сосуществовать тесты:
- `alice.random_number`
- `alice.random_number.ifexp.my_experiment_1`
- `alice.random_number.ifexp.my_experiment_2`

Пример грамматик под экспериментом: [alice/nlu/data/ru/granet/demo/demo.grnt](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/granet/demo/demo.grnt)

### Как быстро протестировать изменения под экспериментом {#fresh-experiment}

Допустим, у вас есть такие формы:

```bash
form alice.my_scenario.turn_on:
    root: включи
form alice.my_scenario.turn_off:
    root: выключи
```

И допустим, вы хотите проверить, как изменится UE2E, если добавить в них слова "вруби" и "выруби". Сделать это можно так.

Добавляем экспериментальные формы:

```bash
form alice.my_scenario.turn_on:
    root: включи
form alice.my_scenario.turn_off:
    root: выключи
form alice.my_scenario.turn_on.ifexp.bg_exp_for_my_changes:
    root: включи | вруби
form alice.my_scenario.turn_off.ifexp.bg_exp_for_my_changes:
    root: выключи | выруби
```

Эти формы можно дописать в те же grnt-файлы. Менять или добавлять тесты Гранета пока не нужно.

Комитим эти правки. Ждём 30 минут. Запускаем UE2E (или что там у вас) и передаём в тестовый прогон такой набор экспериментов:
- `bg_fresh_alice_form=alice.my_scenario.turn_on.ifexp.bg_exp_for_my_changes`
- `bg_fresh_alice_form=alice.my_scenario.turn_off.ifexp.bg_exp_for_my_changes`
- `bg_exp_for_my_changes`

Либо такой набор:
- `bg_fresh_alice_prefix=alice.my_scenario.`
- `bg_exp_for_my_changes`

Флаги `bg_fresh_alice_form=...` или `bg_fresh_alice_prefix=...` заставляют Гранет увидеть в быстрых данных наши экспериментальные формы (пока что под их исходными именами, с суффиксом `.ifexp.bg_exp_for_my_changes`). А флаг `bg_exp_for_my_changes` заставляет Гранет подменить формы `alice.my_scenario.turn_on` и `alice.my_scenario.turn_off` на `alice.my_scenario.turn_on.ifexp.bg_exp_for_my_changes` и `alice.my_scenario.turn_off.ifexp.bg_exp_for_my_changes` соответственно.

### Print Grammar as Experiment {#print-grammar-as-experiment}

Распечатать грамматику в виде строки флага эксперимента можно с помощью скрипта [print_grammar_as_experiment.sh](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/test/granet/print_grammar_as_experiment.sh).

`print_grammar_as_experiment.sh` распечатывает все грамматики из `main.grnt`. Получается очень длинная строчка вида `bg_granet_source_text=H4sIAAAAA...WKMRDsfAAA,`. Эту строчку нужно передать в Мегамайнд в качестве флага эксперимента. Он в свою очередь пробросит её в Бегемот.

К сожалению, строчка со всеми грамматиками стала такой длинной, что перестала влезать в 60 КБ - ограничение на длину запроса в Бегемот. Поэтому распечатывать нужно не все грамматики, а только свои. Эти грамматики "вмержатся" к тем грамматикам, про которые Бегемот уже знает. Распечатать подмножество грамматик можно разными способами:

**Способ 1**

Временно отредактировать файл `main.grnt`, чтобы оставить там только нужные грамматики. И вызвать `print_grammar_as_experiment.sh` без параметров.

**Способ 2**

Передать в `print_grammar_as_experiment.sh` путь к грамматике одного сценария:

```bash
$ARCADIA/alice/nlu/data/ru/test/granet/print_grammar_as_experiment.sh $ARCADIA/alice/nlu/data/ru/granet/quasar/video_rater.grnt
```

Этот скрипт вызывает гранет с такими параметрами:

```bash
$ARCADIA/alice/nlu/granet/tools/granet/granet grammar pack \
    --lang ru \
    --source-dir $ARCADIA/alice/nlu/data/ru/granet \
    --grammar $ARCADIA/alice/nlu/data/ru/granet/quasar/video_rater.grnt \
```

Параметр `--source-dir` - директория, относительно которой резолвятся относительные пути файлов, импортируемых из грамматик. Команда упаковки грамматики выплюнет длинную строку, типа `H4sIAAAAA...WKMRDsfAAA,`. Эту строку (без последней запятой) нужно добавить к префиксу `bg_granet_source_text=` (скрипт print_grammar_as_experiment.sh выдаёт строку сразу с префиксом), и всё это можно передавать в запросе к Мегамайнду как флаг эксперимента.

### Как передать флаг эксперимента {#send-experiment-flag}

Основная инструкция здесь: [Отладка в приложениях](../../testing/scenario-app-testing.md)

#### Telegram-бот Amanda Johnson {#send-amanda}

[Основная инструкция к Аманде](../../testing/amanda.md). Грамматику передаём так:
- Ввести команду `/experiments`
- Нажать кнопку `Добавить`
- Ввести строчку, которую вывел [print_grammar_as_experiment.sh](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/test/granet/print_grammar_as_experiment.sh)

{% note warning %}

К сожалению строчка эксперимента с грамматикой может быть настолько длинной, что превысит ограничение телеграма на длину сообщения.
В таком случае вы не сможете задать эксперимент с помощью бота.
В этом случае можно [поднять мегамайнд с включенным экспериментом](#mm-with-grammar-exp).

{% endnote %}

#### Поднять мегамайнд с включенным экспериментом {#mm-with-grammar-exp}

Чтобы поднять мегамайнд с включенным экспериментом, необходимо добавить в конфигурационный файл мегамайнда (например, [development-конфиг](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/configs/dev/megamind.pb.txt?rev=r9130810)) поле [Experiments](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/library/config/protos/config.proto?rev=r9152138#L235), содержащее необходимый эксперимент.
Важно, что ключом эксперимента должна быть не только строчка `bg_granet_source_text`, но и следующая за этим префиксом закодированная грамматика: `bg_granet_source_text=<закодированная грамматика>`.
Значением эксперимента можно выставить целое значение **1**:

```bash

# ...

Experiments {
    Storage: [
        {
            key: "bg_granet_source_text=<закодированная грамматика>"
            value: {
                Integer: 1
            }
        }
    ]
}

# ...

```

#### vins_client {#send-vins}

Командно-строчная утилита [vins_client](https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/tools/vins_client) умеет посылать запросы в Мегамайнд. Она немного устарела, не умеет работать с контекстом, не поддреживает setrace, поэтому лучше ей не пользоваться. Но для тех, кто к ней привык, вот пример того, как передать ей грамматику:

```bash
ARCADIA=~/data/arcadia
VINS_CLIENT=$ARCADIA/alice/bass/tools/vins_client/vins_client
PRINT_GRAMMAR=$ARCADIA/alice/nlu/data/ru/test/granet/print_grammar_as_experiment.sh
alias runvc='$VINS_CLIENT --vins-url http://megamind.hamster.alice.yandex.net/speechkit/app/pa/'
alias runvce='runvc -e $($PRINT_GRAMMAR)'

echo 'включи номер один' | runvce -s
echo 'назови случайное число от одного до трёх' | runvc -s
```
