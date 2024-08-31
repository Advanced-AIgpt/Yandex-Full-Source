# Как добавить форму

## Добавляем грамматику {#add-grammar}

В Алисе имена интентов (форм) должны выглядеть так: `alice.группа_интент`. Группа — это название сценария или группы интентов. Префикс `personal_assistant.scenarios.` используется только для старых интентов, переехавших из VINS.

В качестве примера добавим интент `alice.light_on`.

Сначала нужно написать грамматику. Удобнее всего это делать в [Гранет-Ферштейн](https://verstehen.n.yandex-team.ru/granet). Допустим, у нас получилось что-то типа такого:

```bash
form alice.light_on:
    filler: $nonsense
    root: включи свет
```

Теперь придётся немного поработать в командной строке.

Берём нужные исходники: `ya make --checkout -j0 alice/nlu/data/ru/granet`

Сохраняем текст грамматики в файл `alice/nlu/data/ru/granet/light_on.grnt` и прописываем его в [alice/nlu/data/ru/granet/main.grnt](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/granet/main.grnt) и [alice/nlu/data/ru/granet/files.lst](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/granet/files.lst).
При создании библиотеки нетерминалов без описания форм, ее нужно добавлять только в [granet/files.lst](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/granet/files.lst).


## Базовая проверка работоспособности грамматики

Проверяем грамматику на какой-нибудь реплике:

```bash
$ARCADIA/alice/nlu/data/ru/test/granet/prepare.sh  # Сборка утилиты granet
$ARCADIA/alice/nlu/data/ru/test/granet/debug_form.sh 'alice.light_on' 'алиса включи свет'
```
Пример [вывода](https://paste.yandex-team.ru/4246573/text).

Скрипт [debug_form.sh](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/test/granet/debug_form.sh) выведет сущности, найденные в реплике (за сущностями он ходит в Бегемот).
А также слоты получившейся формы, дерево разбора и все сматчившиеся нетерминалы.

Также есть скрипт [debug_sample.sh](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/test/granet/debug_sample.sh).
В него не нужно передавать название формы, он только выводит найденные сущности:

```bash
$ARCADIA/alice/nlu/data/ru/test/granet/debug_sample.sh 'алиса включи свет'
```
Пример [вывода](https://paste.yandex-team.ru/4246538/text).


## Добавляем тесты

{% note warning %}

Для комита в Аркадию _наличие тестов обязательно_.

{% endnote %}

Теперь нужно добавить тесты. В Гранете есть следующие виды прекомитных тестов грамматик:
- [small](#small-test) - несколько положительных и отрицательных примеров, пишутся разработчиком грамматики.
- [medium](#medium-test) - канонизированные срабатывания грамматики на датасете из 1 млн. случайных запросов в Алису.
- [tom](#tom-test) - для тестирования и улучшения грамматики на корзинке top of mind, написанной толокерами.
- [custom](#custom-test) - остальные нестандартные тесты.

Для комита в Аркадию грамматика должна иметь medium тесты, а также small или tom тесты.


### Добавляем small-тесты {#small-test}

В small-тесты нужно написать несколько положительных и отрицательных примеров. Примеры нужны не для обучения, а чтобы проверить корректность грамматики. Достаточно 5-10 положительных примеров и немного отрицательных (хотя бы просто "Алиса").

Примеры пишутся в пакеты быстрых юнит-тестов Granet:
- [alice/nlu/data/ru/test/granet/small](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/test/granet/small)
- [alice/nlu/data/tr/test/granet/small](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/tr/test/granet/small)
- [alice/nlu/data/kk/test/granet/small](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/kk/test/granet/small)

Допустим, мы работаем с русским языком (если нет, замените `ru` на ваш язык). И допустим, наш интент — alice.random_number (в русском он уже есть).

Собираем тестовую утилиту и пробуем запустить существующие тесты:

```bash
"$ARCADIA/alice/nlu/data/ru/test/granet/prepare.sh"
"$ARCADIA/alice/nlu/data/ru/test/granet/small/test_all.sh"
```

Создаём файлы:

- `alice/nlu/data/ru/test/granet/small/target/random_number.pos.tsv` — положительные примеры
- `alice/nlu/data/ru/test/granet/small/target/random_number.neg.tsv` — отрицательные примеры

И пишем в них примеры. В один столбец `text`. Столбец `mock` руками вбивать не надо, он добавится позже (см. ниже).

Положительные примеры пишутся с разметкой теггера:

```
text
алиса можешь любое число загадать
загадай число от 'одного'(lower_bound) до 'двадцати трёх'(upper_bound)
```

Прописываем их в конфиг тестового пакета (файл [alice/nlu/data/ru/test/granet/small/config.json](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/test/granet/small/config.json)):

```json
    {
      "name": "random_number",
      "form": "alice.random_number",
      "positive": "target/random_number.pos.tsv",
      "negative": "target/random_number.neg.tsv"
    }
```

И в [alice/nlu/data/ru/test/granet/small/target/ya.make](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/test/granet/small/target/ya.make):

```
FILES(
    ...
    random_number.neg.tsv
    random_number.pos.tsv
    ...
```

После добавления сэмплов нужно обновить их entities:

```bash
# Скрипт обновляет entities для интентов, в имени которых есть подстрока random_number
"$ARCADIA/alice/nlu/data/ru/test/granet/small/update_entities.sh" random_number
```

Entities — это числа, даты, адреса и прочие сущности, найденные в тексте сэмпла. Они ищутся другими, часто тяжеловесными подсистемами, поэтому в тестах Granet они хранятся в закэшированном виде (столбец `mock` в tsv-файле).

При обновлении сущностей утилита granet ходит в хамстер Бегемота: `hamzard.yandex.net:8891/wizard`. Он доступен с ноутбуков, но может быть [недоступен для некоторых дев-машин](https://st.yandex-team.ru/DIALOG-5620). Если не пускают, закажите дырку в панчере. Также есть переменная `GRANET_BEGEMOT`, с помощью которой можно направить granet в другой Бегемот. Пример: `export GRANET_BEGEMOT=reqwizard.yandex.net:8891`.

После обновления entities можно запустить тесты. Либо так:

```bash
"$ARCADIA/alice/nlu/data/ru/test/granet/small/test_all.sh"
```

Либо так (протестируются все языки):

```bash
ya make -rtt "$ARCADIA/alice/nlu/granet/tests/begemot"
```

Если нужно добавить новые примеры в уже существующий tsv-файл, добавляем их так:

```
text<tab>mock
старый сэмпл 1<tab>{закэшированные entities сэмпла 1}
старый сэмпл 2<tab>{закэшированные entities сэмпла 2}
алиса можешь любое число загадать<tab>{}
загадай число от 'одного'(lower_bound) до 'двадцати трёх'(upper_bound)<tab>{}
```

После добавления сэмплов снова обновляем сущности (`update_entities.sh`).


### Дополнительные возможности small-тестов {#small-test-extra}


#### Канонизация слотов

В small-тестах примеры должны содержать разметку слотов:

```
загадай число от 'одного'(lower_bound) до 'двадцати трёх'(upper_bound)
```

Эту разметку не обязательно вбивать руками, её можно заканонизировать. Для этого пишем в tsv-файл только тексты примеров. Далее запускаем:

```bash
"$ARCADIA/alice/nlu/data/ru/test/granet/small/update_entities.sh" random_number
"$ARCADIA/alice/nlu/data/ru/test/granet/small/canonize_tagger.sh" random_number
```

Скрипт `canonize_tagger.sh` читает сэмплы из положительного датасета, вычищает из них старую разметку (если она там была), прогоняет на этих сэмплах грамматику и сохраняет получившуюся разметку обратно в датасет.


#### Тестирование значений слотов

В разметке можно указать ожидаемые значения слотов (пример в `lower_bound`), а также ожидаемые типы значений слотов (пример в `upper_bound`):

```
загадай число от 'одного'(lower_bound:1) до 'двадцати трёх'(upper_bound/sys.type:23)
```

Чтобы тесты проверяли значения слотов, нужно включить опции `print_slot_values` и/или `print_slot_types`:

```json
    {
      "name": "random_number",
      "form": "alice.random_number",
      "positive": "target/random_number.pos.tsv",
      "negative": "target/random_number.neg.tsv",
      "print_slot_values": true,
      "print_slot_types": true
    }
```

Эти опции также влияют на результат скрипта `canonize_tagger.sh` - он будет добавлять в разметку не только названия слотов, но и соответствующие им значения и типы.

Например, если слот `calendar_date` имеет тип `sys.datetime`, то разметка будет выглядеть не так:

```
привет алиса какое 'завтра'(calendar_date) число
```

а так:

```
привет алиса какое 'завтра'(calendar_date/sys.datetime:{"days":1,"days_relative":true}) число
```


### Добавляем medium-тесты {#medium-test}

{% note info %}

Для коммита в Аркадию _наличие этих тестов обязательно_. Они нужны для выявления неожиданных false-positive.

{% endnote %}

{% cut "Лирическое отступление" %}

Раньше эти тесты были полезны ещё и для отладки грамматики, так как с помощью них можно "грепнуть" большую выборку запросов к Алисе (100 000 запросов обрабатывается за несколько секунд) и посмотреть, на что матчится ваша грамматика. Но теперь эта процедура обрела более удобный интерфейс: [Гранет-Ферштейн](https://verstehen.n.yandex-team.ru/granet).

Этот вариант пока доступен только для русского языка. Для него есть пул готовых датасетов разных объёмов (из 5^10, 6^10 и 7^10 случайных запросов в Алису), в которых entities уже закэшированы. Их удобно использовать для создания своих датасетов. Лежат они здесь: [alice/nlu/data/ru/test/pool](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/test/pool/)

Тестовые пакеты для больших выборок лежат здесь:
- [alice/nlu/data/ru/test/granet/medium](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/test/granet/medium) — пакет для medium юнит-тестов. Любая ошибка на этом пакете роняет тест.
- [alice/nlu/data/ru/test/granet/quality/main](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/test/granet/quality/main) — пакет для локального замера качества. Ошибки выводятся в виде таблицы. Пока запускается только локально.

{% endcut %}

Будем работать с пакетом [alice/nlu/data/ru/test/granet/medium](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/test/granet/medium).

Сразу добавляем `light_on.tsv` в [alice/nlu/data/ru/test/granet/medium/target/ya.make](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/test/granet/medium/target/ya.make). Этого файла пока нет, он будет получен позже с помощью канонизации.

Дописываем наш интент в конфиг тестового пакета — файл [alice/nlu/data/ru/test/granet/medium/config.json](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/test/granet/medium/config.json):

```json
    {
      "name": "light_on",
      "form": "alice.light_on",
      "base": "../../pool/alice6v2.tsv",
      "positive": "target/light_on.tsv"
    }
```

Здесь написано следующее:
- `"name": "light_on"` — имя test-case, используется в при выводе результатов.
- `"form": "alice.light_on"` — полное имя интента.
- `"base": "../../pool/alice6v2.tsv"` — источник сэмплов. Сэмпл — текст запроса с найденными в нём сущностями (даты, числа, nonsense, custom entities, адреса и т.п.). В папке [alice/nlu/data/ru/test/pool](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/test/pool) лежат неразмеченные датасеты разного объёма, которые можно использовать в качестве источника сэмплов для своих датасетов. Конкретно этот датасет содержит 1 миллион случайных запросов.
- `"positive": "target/light_on.tsv"` — датасет с положительными примерами. Его пока нет.

Также в config.json есть секция default с общими для всех test-case параметрами:
- `"negative_from_base_count": 10000` — в качестве отрицательных примеров взять 10000 сэмплов из датасета base (минус датасет positive).

Для создания датасета `target/light_on.tsv` запускаем скрипт [alice/nlu/data/ru/test/granet/medium/canonize.sh](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/test/granet/medium/canonize.sh).

```bash
"$ARCADIA/alice/nlu/data/ru/test/granet/medium/canonize.sh" alice.light_on
```

Этот скрипт создаёт positive-датасет для интентов, в названии которых содержится `alice.light_on`. Датасет создаётся канонизацией текущих результатов грамматики на base-датасете (в нашем случае из `alice6v2.tsv`).

В результате должен появиться файл `alice/nlu/data/ru/test/granet/medium/target/light_on.tsv`. В нём будут лежать все сэмплы из `alice6v2.tsv`, сматчившиеся с `alice.light_on`. Его желательно просмотреть глазами, чтобы убедиться, что всё ок. Если сэмплов получилось слишком много, можно заменить `alice6v2.tsv` (1 млн. сэмплов) на `alice5v2.tsv` (100 тыс. сэмплов) и перегенерировать `light_on.tsv` (для этого его нужно удалить).

Теперь (опционально) можно пробовать запустить тест на отредактированном пакете:

```bash
"$ARCADIA/alice/nlu/data/ru/test/granet/medium/test_all.sh"
```

Результаты этого теста будут записаны в директорию `alice/nlu/data/ru/test/granet/medium/results/current-date-time`. Но так как наш пакет предназначен для юнит-тестов, здесь мало что интересного.

{% note info %}

Если в medium-тестах маленькое покрытие, вы можете собрать более узкий корпус и залить его как custom-тест.
Подробнее про custom-тесты — в разделе [Работа со своими датасетами](custom-datasets.md).

{% endnote %}


#### Обновление medium-тестов {#medium-update}

После изменений грамматик перед комитом нужно переканонизировать medium-тесты, затронутые вашими изменениями. Делается это так:

```bash
"$ARCADIA/alice/nlu/data/ru/test/granet/medium/canonize.sh" alice.light_on
```

Если ваши изменения затрагивают много грамматик (например, вы что-то поправили в common-нетерминалах: [](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/granet/common)), вам скорее всего придётся переканонизировать все medium-тесты. Для этого в `canonize.sh` можно передать пустую строку:

```bash
"$ARCADIA/alice/nlu/data/ru/test/granet/medium/canonize.sh" ""
```

Полная переканонизация работает слишком долго (часа два). Гораздо быстрей переканонизировать только падающие тесты. Делается это с помощью скрипта `canonize_failed.sh`:

```bash
"$ARCADIA/alice/nlu/data/ru/test/granet/medium/canonize_failed.sh"
```


### Написание грамматики с помощью корзинки top of mind {#tom-test}

Написание грамматики можно значительно ускорить, если у нас есть корзинка top of mind.

Опишем процедуру на примере интента `alice.equalizer.less_bass` ("меньше басов"). На выходе у нас получится:
- Корзинка top of mind на YT: [//home/alice-ue2e/baskets_from_logs/baskets/ALICETOM-70__mysov-gr__less_bass/basket_dev](https://yt.yandex-team.ru/hahn/navigation?path=//home/alice-ue2e/baskets_from_logs/baskets/ALICETOM-70__mysov-gr__less_bass/basket_dev).
- Грамматика: [alice/nlu/data/ru/granet/equalizer/less_bass.grnt](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/granet/equalizer/less_bass.grnt).
- Тестовый пакет для гранета: [alice/nlu/data/ru/test/granet/tom/equalizer_less_bass](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/test/granet/tom/equalizer_less_bass).


#### Добавляем заготовку грамматики

Добавляем простенькую грамматику примерно такого вида:

```bash
import: common/all.grnt

form alice.equalizer.less_bass:
    lemma: true
    root:
        [$Less+ $Bass+]
    filler:
        $nonsense
        $Common.Filler
        $Common.WayToCallAlice
        $Common.LeftFiller
$Less:
    убавь

$Bass:
    басы
```

Процедура добавления грамматики описана выше ([Добавляем грамматику](#add-grammar)).


#### Собираем TOM на Толоке

Сейчас есть два разных процесса:
1. [Top-of-mind корзинки, где толокеры придумывают запросы](https://wiki.yandex-team.ru/alice/analytics/newscenariousbasket/tom/)
2. [Корзинки по логам](https://wiki.yandex-team.ru/alice/analytics/newscenariousbasket/basketfromlogs/)

Вам нужен первый вариант. Но вообще, можно использовать произвольные корзинки.
Утилита создания гранетного пакета использует только колонку `text` и опционально `is_negative_query`.

В результате получаем корзинку [//home/alice-ue2e/baskets_from_logs/baskets/ALICETOM-70__mysov-gr__less_bass/basket_dev](https://yt.yandex-team.ru/hahn/navigation?path=//home/alice-ue2e/baskets_from_logs/baskets/ALICETOM-70__mysov-gr__less_bass/basket_dev).


#### Создаём тестовый пакет TOM

Запускаем на своей машине скрипт [alice/nlu/data/ru/test/granet/tom/create.sh](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/test/granet/tom/create.sh):

```bash
$ARCADIA/alice/nlu/data/ru/test/granet/tom/create.sh \
    equalizer_less_bass \
    alice.equalizer.less_bass \
    //home/alice-ue2e/baskets_from_logs/baskets/ALICETOM-70__mysov-gr__less_bass/basket_dev
```

Параметры:
- `equalizer_less_bass` - название создаваемого пакета. То есть имя подпапки в [alice/nlu/data/ru/test/granet/tom](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/test/granet/tom).
- `alice.equalizer.less_bass` - имя формы.
- `//home/alice-ue2e/baskets_from_logs/baskets/ALICETOM-70__mysov-gr__less_bass/basket_dev` - корзинка top of mind. Скрипт полагается на то, что в корзинке есть колонка `text`. А также опциональная колонка `is_negative_query`. Если её нет, то все примеры будут считаться положительными.

Скрипт создаст тестовый пакет `alice/nlu/data/ru/test/granet/tom/equalizer_less_bass`.


#### Пишем грамматику на основе TOM

Запускаем тестирование на нашем пакете:

```bash
$ARCADIA/alice/nlu/data/ru/test/granet/tom/equalizer_less_bass/test_all.sh
```

Получим примерно такой вывод:

```
...
================================================================================================================
By weight                       Precis  Recall  Tagger      Excess        Lost  Target   Total  TimeAvg  TimeMax
----------------------------------------------------------------------------------------------------------------
canonized_alice                  1.000   1.000   1.000      0   0%      0   0%       0  155019     2 µs    97 µs
canonized_tom_false_neg          1.000   1.000   1.000      0   0%      0   0%      48    5470     4 µs    57 µs
canonized_tom_false_pos          1.000   1.000   1.000      0   0%      0   0%       1    2866     3 µs    62 µs
tom_quality                      1.000   0.292   0.292      1   0%   5422  71%    7663   21244     4 µs    62 µs
================================================================================================================
Suggests for tom_quality:
  Count  Blocker     Sample                          Nonterminals
   1844  убери       убери басы                      $Common.CancelRemoveReset $Common.Hide $Common.Remove
   1280  сделай      сделай басы потише              $Common.MakeItSoThat $Common.Make $Common.Set
    813  уменьши     уменьши бас
    528  меньше      меньше басов
    222  поменьше    басов поменьше                  $Common.PersonsAll $Common.Children $Common.Short $Common.Small
    129  убрать      убрать басы                     $Common.CancelRemoveReset $Common.Hide $Common.Remove
    113  низкие      убавь низкие частоты
     50  можно       басы можно убрать               $Common.Agree
     50  уменьшить   ты можешь уменьшить басы
     41  потише      бас потише
     35  сделать     можешь сделать басы потише      $Common.MakeItSoThat $Common.Make $Common.Set
     31  тише        басы тише сделай
     29  колонках    убавь басы на колонках          $Common.SmartDevice $Common.SmartSpeaker
     24  можешь      ты можешь убавить бас           $Common.Agree $Common.Lets $Common.Can
     22  немножко    убавь немножко басы             $Common.Little
     22  чуть        басы чуть тише                  $Common.Little
     18  колонке     убавь басы на колонке           $Common.SmartDevice $Common.SmartSpeaker
     17  ниже        чуть ниже басы
     16  сбавь       сбавь бас
     15  снизь       снизь басы
    ...
```

В начале идёт таблица с текущим качеством классификатора. Нужно смотреть строчку tom_quality: пока что recall 0.292.

Дальше идё таблица с подсказками для улучшения грамматики. Эта таблица строится благодаря флагу [collect_blockers](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/test/granet/tom/equalizer_less_bass/config.json?rev=r9289665#L13). В этом режиме парсер пытается определить, из-за чего на сматчились положительные примеры.

Колонки:
- `Blocker` - причина, по которой не сматчился пример (см. ниже).
- `Count` - количество примеров с такой причиной.
- `Sample` - один из примеров с такой причиной.
- `Nonterminals` - список common-нетерминалов, в которых есть слово из колонки `Blocker`. Common-нетерминалы берутся отсюда: [alice/nlu/data/ru/granet/common/grammar_suggest.grnt](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/granet/common/grammar_suggest.grnt).

Причины могут быть следующие:
- Слово - парсер не смог сматчить какое-то слово, потому что его нет в грамматике. В этом случае можно добавить в грамматику это слово (например, добавить в `$Less` слово `убери`), но лучше добавить не само слово, а один из предложенных common-нетерминалов (например, `$Common.Remove`).
- `WEAK_TEXT` - парсер дошёл до конца текста примера, но при этом грамматика наложилась не полностью. Такая ситуация типична, если в грамматике используется точка, которая накладывается на любое слово: `[.* убери басы]`.
- `STATE_LIMIT` - при обработке сэмпла произошёл комбинаторный взрыв, и парсеру пришлось выбросить некоторые ветви перебора.

Причины отсортированы по убыванию частотности. Устраняем первую причину и заходим на следующий круг.

С каждой итерацией recall грамматики будет расти, а табличка подсказок постепенно таять, и в конце мы получим что-то типа такого:

```
================================================================================================================
By weight                       Precis  Recall  Tagger      Excess        Lost  Target   Total  TimeAvg  TimeMax
----------------------------------------------------------------------------------------------------------------
canonized_alice                  1.000   1.000   1.000      0   0%      0   0%       0  155019     3 µs   380 µs
canonized_tom_false_neg          0.009   1.000   1.000   5415  99%      0   0%      48    5470    59 µs   285 µs
canonized_tom_false_pos          0.033   1.000   1.000     29  97%      0   0%       1    2866    27 µs   207 µs
tom_quality                      0.992   0.999   0.999     59   1%      7   0%    7663   21244    32 µs   272 µs
================================================================================================================
Suggests for tom_quality:
  Count  Blocker     Sample                          Nonterminals
      3  погромче    убери басы и сделай погромче    $Common.Aloud
      1  WEAK_TEXT   убери немножко низкие
      1  арию        убавь басы и включи арию
      1  семь        звук семь убери басы
      1  стерео      басов поменьше стерео побольше
```

Небольшой лайфхак. Работу `test_all.sh` можно значительно ускорить, если временно удалить из `main.grnt` все грамматики кроме вашей и `common/grammar_suggest.grnt` (используется для построения колонки `Nonterminals`).


#### Перед комитом

Перед комитом нужно заканонизировать текущие ошибки:

```bash
$ARCADIA/alice/nlu/data/ru/test/granet/tom/equalizer_less_bass/canonize.sh ''
```

Добавляем в комит папку `alice/nlu/data/ru/test/granet/tom/equalizer_less_bass`:

```bash
arc add $ARCADIA/alice/nlu/data/ru/test/granet/tom/equalizer_less_bass
```

Примечание: В папке `equalizer_less_bass` будет находиться подпапка `results` - она в комите не нужна. Но она в него и не добавится, благодаря этому [.arcignore](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/test/granet/.arcignore).


### custom-тесты {#custom-test}

Custom-тесты лежат здесь: [alice/nlu/data/ru/test/granet/custom](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/test/granet/custom). Сюда свалены разные нестандартные тесты грамматик. Например, тесты фреймов, которые зависят не только от текста запроса, но и его контекста - названия устройств умного дома пользователя, телефонная книга и т.п. Контекст задаётся с помощью колонки wizextra (см. [alice/nlu/data/ru/test/granet/custom/iot](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/test/granet/custom/iot)).


## Комитим

Если всё хорошо, комитим изменения. Примерно через полчаса после комита Бегемот увидит вашу грамматику, и её можно будет начать использовать в Алисе. Как это делается, написано в следующем разделе:
[Тестирование грамматики в Алисе](alice-test.md).
