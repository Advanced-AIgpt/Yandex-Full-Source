# Здесь будет дока по clickhouse

ClickHouse - столбцовая СУБД, которая используется для онлайн обработки аналитических запросов. Подробнее можно почитать здесь [здесь](https://clickhouse.tech/docs/ru/). По факту, ClickHouse это то место, где лежат все аналитические данные, собираемые AppMetrik-ой, в сыром виде. Для того, чтобы как-то получат и обрабтывать эти данные, в ClickHouse используется YQL.

YQL - язык запросов к БД, сделаный в яндексе. Во много похож на SQL, но со своими плюшками. Документация по YQL находится здесь [здесь](https://yql.yandex-team.ru/docs/).

Одно из применений ClickHouse у нас - это хранение событий здоровья, с помощью которых мы можем понимать, насколько стабильно ведёт себя какой-то функционал на проде, раскапывать баги и справляться с факапами

## Как события здоровья доезжают до ClickHouse

![схема](https://jing.yandex-team.ru/files/eugenemartins/clickhouse-transport-scheme.png)

События здоровья из приложения попадают в YT, оттуда marketParser перекладывает события здоровья маркета в market_errors и market_client_timers. События доезжают в среднем за 4 минуты. В графане, которая у нас используется чтобы строить графики по событиям здоровья, и, соответственно, также ходит в ClickHouse, среднее время доставки событий можно посмотреть вверху в блоке:

![скрин графаны](https://jing.yandex-team.ru/files/eugenemartins/grafana-screen.png)

## Таблицы ClickHouse для мобильного разработчика

### **market_client_timers - данные событий скорости на клиентах**

**Android:**

События скорости мы обычно отправляем через SpeedService. Параметры методов startEvent/stopEvent маппятся в таблицу следующим образом:

**Как параметры АПИ аналитики в коде маппятся на таблицы ClickHouse**
|HealthEvent (legacy API)              |SpeedService             |ClickHouse              |
|--------------------------------------|-------------------------|------------------------|
|name                                  |name                     |name                    |
|portion                               |portion                  |portion                 |
|level                                 |level                    |-                       |
|MetrikaPerfomanceData.startTime       |startTime                |start_time/start_time_ms|
|MetrikaPerfomanceData.timeStamp       |timeStamp                |timestamp/timestamp_ms  |
|MetrikaPerfomanceData.duration        |- (считается под капотом)|duration_ms             |
|MetrikaPerfomanceData.isMainComponent]|isMainComponent          |-                       |
|MetrikaPerfomanceData.coldShow        |coldShow                 |-                       |
|                                      |screenId                 |                        |


Также в событии скорости можно пропихнуть доп. параметры через JsonBuilder (как и для событий здоровья). Эти параметры затем прикапываются в два отдельных столбца:

-   extra_keys - массив с ключами параметров
-   extra_values - массив со значениями параметров

Про них будет ниже.

**IOS:**

TODO

### **market_errors - данные событий здоровья на клиентах (и не только)**

**Android:**

Данные, которые пишем в info, складываются в виде JSON в поля extra_keys и extra_values, иногда может дублироваться в отдельном столбце stack_trace.

Часть параметров событий здоровья складывает в отдельные столбцы таблицы market_errors

**Как параметры АПИ аналитики в коде маппятся на таблицы ClickHouse**
|HealthEvent (legacy API)              |HealthFacade             |ClickHouse              |
|--------------------------------------|-------------------------|------------------------|
|name                                  |eventName                |code                    |
|level                                 |healthLevel              |level                   |
|-                                     |contur                   |-                       |
|requestId                             |-                        |-                       |
|Info(messageStr: String)              |-                        |message                 |
|Info(stackTrace: String)              |-                        |stack_trace             |

Оставшаяся часть параметров прикапывается опять же в два знакомых столбца, хранящие строковые массивы:

-   extra_keys - массив с ключами параметров
-   extra_values - массив со значениями параметров

У разных событий набор параметров может отличаться, поэтому использование такой гибкой структуры как строковый массив кажется более удобным решением, чем заводить отдельные столбцы под каждый параметр

**Параметры АПИ здоровья маппятся в extra_keys и extra_values**
|HealthEvent (legacy API)              |HealthFacade                              |ClickHouse(extra_keys)                           |
|--------------------------------------|------------------------------------------|-------------------------------------|
|portion                               |healthPortion                             |portion                              |
|Info(любые поля наследников)          |json, добавляемый через eventParamSupplier|любые дополнительные значения массива|

**IOS:**

TODO

### Немного про HealthEvent (android only)

У старого HealthEvent есть параметры, которые используются для указания, является ли HealthEvent событием здоровья или скорости:

-   type - может принимать значения SPEED либо HEALTH. Если SPEED, то это событие скорости, и попадает в таблицу market_client_timers, если HEALTH - это событие здоровья и попадает в market_errors
-   metrikaData - параметры с замерами скорости, если событие используется как событие скорости (type == SPEED). Если у событие с metrikData указать параметр type как HEALTH - metrikData игнорируется.

### Как доставать данные из таблиц

Чтобы доставать данные для всех этих таблиц, мы пишем запросы на YQL в специальном [веб-интерфейсе](https://yql.yandex-team.ru/). Его синтаксис во многом похож на SQL, но с дополнительными плюшками. Так может выглядеть обычный запрос на получение инфы о конкретной ошибке за сегодня:

```sql
use marketclickhouse;

select 
toDateTime("timestamp"),
 *
from market.market_errors 
WHERE
  "date" = today()
  and platform = 'android' -- не забудь проставить нужную платформу
  and service = 'market_front_bluetouch'
  and code = 'CHECKOUT_SELECT_PICKUP_POINT_ERROR'
order by timestamp desc

```

Давайте разбираться, что тут вообще творится:

```sql
use marketclickhouse;

```

Здесь мы указываем, что будем работать маркетным кластером кликхауза, в нём у нас находятся все наши таблички.

```sql
select 
toDateTime("timestamp"),
 *

```

Мы указываем что хотим достать все столбцы из таблицы, а также хотим добавить в результат запроса столбец, который форматирует данные из столбца timestamp в человекочитаемый вид через функцию `toDateTime("timestamp")`


> Функция `toDateTime("timestamp")` - это лишь одна из многих функций, доступных разработчику при написании YQL-запросов. Посмотреть, какие ещё функции есть, и найти что-то подходящее для себя можно в [документации ClickHouse по функциям](https://clickhouse.tech/docs/ru/sql-reference/functions/)


```sql
from market.market_errors 

```

Здесь всё просто, мы говорим, что запрос будет брать данные из таблицы `market.market_errors`

```sql
WHERE
  "date" = today()
  and platform = 'android' -- не забудь проставить нужную платформу
  and service = 'market_front_bluetouch'
  and code = 'CHECKOUT_SELECT_PICKUP_POINT_ERROR'

```

В конструкции `WHERE` мы фильтруем данные, явно указывая, что нас интересуют только записи за сегодня (`today()` - ещё одна удобная функция), что мы хотим только данные по платформе андроид, относящиеся к сервису `market_front_bluetouch` (это обозначение именно фронтовых логов приложения маркета, в market_errorsлежат данные не только этого сервиса, но и многих других), и что мы хотим посмотреть именно на событие `CHECKOUT_SELECT_PICKUP_POINT_ERROR`

> В примере мы смотрим данные только по одному событию, но никто не мешаем нам перечислить несколько через оператор `or`

```sql
order by timestamp desc

```

Здесь мы говорим что полученный результат надо отсортировать по `timestamp` по убыванию.

Если вы когда-нибудь сталкивались с необходимостью писать SQL-запросы, то данный код не должен вызвать у вас проблем с пониманием.

Из этого примера в целом понятно как делать запросы в таблицу, но как быть, если нужно достать не просто данные из столбцов, а например, ещё какие-то поля из массива параметров, который мы отправили с приложения? (речь о данных из таблиц extra_keys и extra_values). Давайте разбираться.

Вот пример запроса, который лезет в параметры событий. Здесь мы пытаемся получить все ошибки, который были сегодня у пользователя с определённым UUID:

```sql
use marketclickhouse;

select 
 toDateTime(timestamp) as time,
 code,
 level,
 message,
 stack_trace,
 extra_keys,
 extra_values
from market.market_errors 
WHERE
  "date" = today()
  and platform = 'android'
  and arrayElement(extra_values, indexOf(extra_keys, 'uuid')) == '21f7a163574d41958600e78f98cc2c15'
  and service = 'market_front_bluetouch'
ORDER BY timestamp DESC

```

Самый большой интереса для нас в этом месте представляет конструкция `WHERE`, все остальные операторы здесь делают примерно то же самое, что и в предыдущем примере.

```sql
WHERE
  "date" = today()
  and platform = 'android'
  and arrayElement(extra_values, indexOf(extra_keys, 'uuid')) == '21f7a163574d41958600e78f98cc2c15'
  and service = 'market_front_bluetouch'

```

Фильтрация по дате, платформе и сервису здесь такие же как и раньше, но вот конструкция для получения UUID выглядит странно, свежо и непонятно

```sql
arrayElement(extra_values, indexOf(extra_keys, 'uuid')) == '21f7a163574d41958600e78f98cc2c15'

```

Давайте разбираться. Вспоминаем, что у нас прикопано в столбцах `extra_keys` и `extra_values`

-   `extra_keys` - массив строк, где каждая строка - это ключ параметра события
-   `extra_values` - массив строк, где каждая строка - это значение параметра события

Соответственно, в нашем примере uuid в этих столбцах будет прикопан следующим образом

`extra_keys`

```json
[
	"...",
	"uuid"
	"..."
]

```

`extra_values`

```json
[
	"...",
	"21f7a163574d41958600e78f98cc2c15",
	"..."
]

```

Индекс, по которому лежит ключ uuid в первом массиве и его значение во втором массиве совпадают, поэтому чтобы достать это значение нам нужно:

1.  Узнать индекс ключа uuid в массиве `extra_keys`
2.  Достать по этому индексу значение uuid из массива `extra_values`

Теперь вернёмся к странной функции, и посмотрим, как эти шаги реализуются в ней

1.  Узнать индекс ключа uuid в массиве `extra_keys`

```sql
indexOf(extra_keys, 'uuid')

```

Функции `indexOf` скармливаем столбец `extra_keys`, а также - индекс какого значения хотим узнать - uuid. Соответственно, функция вернёт нам индекс.

1.  Достать по этому индексу значение uuid из массива `extra_values`

```sql
arrayElement(extra_values, indexOf(extra_keys, 'uuid'))

```

Функции `arrayElement` скармливаем столбец `extra_values` и результат работы `indexOf`. Функция берёт полученный индекс, и достаёт по нему значение массива.

В обоих случаях, первым параметром в функции идёт массив, с которым функция будет работать, а вторым - специфичный параметр, который уже зависит от самой функции.

Далее, результат работы этой функции просто сравнивается с нужным нам UUID. Всё! Ничо сложного =).

**Ссылки на подробную документацию функций:**

-   [arrayElement](https://clickhouse.tech/docs/ru/sql-reference/functions/array-functions/#arrayelementarr-n-operator-arrn)
-   [indexOf](https://clickhouse.tech/docs/ru/sql-reference/functions/array-functions/#indexofarr-x)


### Обучалка по ClickHouse

В целом, того что описано выше достаточно чтобы выполнять какие-то базовые операции со здоровьем в ClickHouse, но для всех кейсов базовых операций может не хватать. В таком случае, нужно немного глубже разобраться в том, какие возможности предоставляет ClickHouse, и для этого у него есть своя обучалка. Как в неё попасть:

1.  Открываем [онлайн редактор YQL](https://yql.yandex-team.ru/)
2.  Ищем слева иконку tutorial

![Иконка tutorial](https://jing.yandex-team.ru/files/eugenemartins/yql-tutorial-1.png)

3.  В открывшейся панели сверху тыкаем ClickHouse

![Тыкаем clickhouse](https://jing.yandex-team.ru/files/eugenemartins/yql-tutorial-2.png)

4.  Берём чай-кофе-пиво и начинаем проходить туториалы.

![Видимо туториалы](https://jing.yandex-team.ru/files/eugenemartins/yql-tutorial-3.png)
