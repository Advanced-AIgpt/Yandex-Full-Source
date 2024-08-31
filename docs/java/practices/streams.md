Stream появился в Java 8 и позволяет писать более лаконичный и читабельный код для обработки коллекций
по сравнению с циклами. При этом стримы в целом являются менее производительными.

TL;DR:
1) Стримы являются предпочтительным способом обработки коллекций.
2) В публичном интерфейсе класса не следует возвращать или принимать стримы, 
вместо них рекомендуется использовать коллекции или итераторы.
3) В критичных с точки зрения производительности местах стоит использовать не стримы, а
старые добрые for-циклы. Таким образом, в общих библиотеках рекомендуется использовать ```for```, 
так как неизвестно кто и в каких условиях будет использовать библиотеку.

### Основное
- Каждый новый метод стрима рекомендуется писать на отдельной строке. Не рекомендуется писать однострочные стримы,
кроме совсем тривиальных случаев: ``` Set<Long> ids = list.stream().map(Entity::getId).collect(Collectors.toSet());```
- В большинстве случаев method reference лучше чем аналогичная ему лямбда:
{% cut "example" %}
```
// use it
list.stream()
    .map(this::process)
    .collect(Collectors.toList());
// instead of
list.stream()
    .map(element -> process(element))
    .collect(Collectors.toList());
```
{% endcut %}

- Не рекомендуется передавать лямбду на несколько строк в методы стрима.
Вместо этого лучше вынести её код в отдельный метод с читаемым названием.

{% cut "example" %}
```
// use it
list.stream()
        .map(element -> constructResponse(element, nameByElements))
        .collect(Collectors.toList());
// instead of
list.stream()
    .map(element -> {
            String name = nameByElements.get(element);
            return new Response(element.getId(), element.getType(), name);
    })
    .collect(Collectors.toList());
```
{% endcut %}

- Рекомендуется делать внутренние лямбды стримов "чистыми" без побочных эффектов. 
Например, вместо изменения AtomicInteger внутри лямбды в стриме лучше написать обычный for.
- Осторожнее с ```Collectors.toMap()```. При наличии одинаковых ключей будет выброшен 
```IllegalStateException```. Если во входных данных допускаются одинаковые ключи, то стоит явно передавать третьим
аргументом ```mergeFunction```, например: ```toMap(A::getX(), A::getY(), (first, second) -> first)```.

### Когда стримы читаются хуже циклов
- Например, когда нужно сделать декартово произведение двух коллекций/массивов:

{% cut "example" %}
```
// streams
List<AB> result = Stream.of(A.values())
        .flatMap(a -> Stream.of(B.values()).map(b -> new AB(a, b)))
        .collect(Collectors.toList());
// for
var result = new ArrayList<AB>();
for (A a: A.values()) {
   for (B b : B.values()) {
       result.add(new AB(a, b));
   }
}
```
{% endcut %}

Вложенные циклы почти всегда читаются сильно лучше вложенных стримов.

- Когда помимо самого объекта важен ещё его индекс. Например, посчитать сумму элементов, которые делятся на три
и индекс которых в списке тоже делится на 3.

{% cut "example" %}
```
// streams
List<Integer> list = List.of(1, 2, 3, 6, 9);
long sum = IntStream.rangeClosed(0, list.size())
        .filter(index -> index % 3 == 0)
        .map(list::get)
        .filter(element -> element % 3 == 0)
        .mapToLong(Long::valueOf)
        .sum();
// for
long sum = 0;
for (int i = 0; i < list.size(); i++) {
    int element = list.get(i);
    if (i % 3 == 0 && element % 3 == 0) {
        sum += element;
    }
}
```
{% endcut %}

- Когда нужно изменять внешние переменные.
{% cut "example" %}
```
// streams
AtomicInteger unknownCount = new AtomicInteger(0);
var ids = new ArrayList<Long>();
Map<Long, String> names = ids.stream()
        .collect(Collectors.toMap(
                id -> id,
                id -> loadName(id).orElseGet(() -> "Unknown #" + unknownCount.incrementAndGet())
        ));
// for
int unknownCount = 0;
var ids = new ArrayList<Long>();
var names = new HashMap<Long, String>();
for (long id : ids) {
    String name = loadName(id).orElse(null);
    if (name == null) {
        unknownCount++;
        name = "Unknown #" + unknownCount;
    }
    names.put(id, name);
}
```
{% endcut %}

### StreamEx
Библиотека [StreamEx](https://github.com/amaembo/streamex) разрешена к использованию в Аркадии. 
Классы ```StreamEx``` и ```EntryStream``` реализуют интерфейс ```Stream``` и предоставляют большое 
количество удобных методов. Основное преимущество библиотеки — лёгкая работа со стримами для мап по key-value:
```mapKeys(key -> ...)```, ```mapKeyValue((key, value) -> ...)```, ```mapToValue((key, value) -> ...)```.
Подробнее можно почитать и посмотреть примеры кода можно в 
[github-репозитории](https://github.com/amaembo/streamex) библиотеки. Решение по использованию StreamEx в 
своём сервисе/проекте каждая команда принимает самостоятельно.

{% cut "Стрим по мапе" %}
```
return EntryStream.of(params)
        .filterValues(Objects::nonNull)
        .sortedBy(Map.Entry::getKey)
        .mapKeyValue((paramType, value) -> paramType.getName() + ": " + value)
        .joining("; ");```
```
{% endcut %}

{% cut "Найти топ-100 самых частых чисел в списке" %}
```
StreamEx.of(list)
        .sorted()
        .mapToEntry(i -> 1) // переходим к стриму кортежей вида (число, 1)
        .collapseKeys(Integer::sum) // группируем по соседним одинаковым ключам и получаем стрим кортежей (число, количество вхождений)
        .reverseSorted(Map.Entry.comparingByValue())
        .keys()
        .limit(100)
        .toList();


// pure streams. Приходится обязательно указывать конкретные параметры generic-ов,
// для того чтобы Java смогла вывести правильный тип компаратора
list.stream()
        .collect(Collectors.groupingBy(Function.identity(), Collectors.counting()))
        .entrySet().stream()
        .sorted(Map.Entry.<Integer, Long>comparingByValue().reversed())
        .map(Map.Entry::getKey)
        .limit(100)
        .collect(Collectors.toList());
```
{% endcut %}

{% cut "Декартово произведения значений двух енумов" %}
```
List<AB> result = StreamEx.of(A.values())
        .cross(B.values())
        .mapKeyValue(AB::new)
        .toList();
```
{% endcut %}

{% cut "Фильтрация по индексу" %}
```
long sum = EntryStream.of(list)
        .filterKeys(index -> index % 3 == 0)
        .filterValues(element -> element % 3 == 0)
        .values()
        .map(Long::valueOf)
        .sum();

```
{% endcut %}

### Parallel streams
Метод ```Stream::parallel``` следует использовать с большой осторожностью:
- Все операции при этом по умолчанию будут выполняться в общем ForkJoinPool'e => нельзя использовать его для блокирующих операций
- Не все методы хорошо распараллеливаются: например, использование ```.limit(n)``` 
убьёт всю выгоды от использования параллельного стрима.
- В большинстве случаев выигрыш от использования параллельного стрима будет незначительным либо из-за малого
размера коллекции либо из-за того, что параллельная обработка не является бутылочным горлышком:
обычно им являются IO операции.

Жизненный пример использования:
в map-reduce job для YT выкачиваются батчами документы из динтаблицы — тогда IO в рамках одного кластера работает очень
быстро, а в обработке присутствует тяжелое сжатие данных и тогда параллельные стримы заметно экономят время обработки,
если выделить джобе ядра.

### Kotlin
Аналогом ```Stream``` в Kotlin'е является ```Sequence```. Также аналогичные методы есть непосредственно у котлиновских
стандартных коллекций. Большинство методов ```Sequence``` не создают промежуточных коллекций, исключением являются 
методы ```sorted...```, ```distinct...```, ```chunked...``` и ```minus```.
Методы коллекций всегда возвращают ```kotlin.collections.List```.

Рекомендации для Kotlin'а:
- Для простой обработки коллекции, результатом которой является список нужно использовать методы коллекций:

{% cut "example 1" %}
```
// use this
val evenNumbers = listOf(1, 2, 6).filter { x -> x % 2 == 0 }
// instead of this
val evenNumbers = listOf(1, 4, 6).asSequence().filter { x -> x % 2 == 0 }.toList()
```
{% endcut %}

{% cut "example 2" %}
```
// use this
val distinctEvenNumbers = listOf(1, 2, 2, 6).filterTo(HashSet(), { x -> x % 2 == 0 })
// instead of this
val distinctEvenNumbers = listOf(1, 2, 2, 6).asSequence()
        .filter { x -> x % 2 == 0 }
        .toSet() 
```
{% endcut %}

Либо если результатом является единственное значение:
{% cut "example 3" %}
```
// use this
val lastEven = listOf(1, 2, 6).findLast { x -> x % 2 == 0 }
// instead of this
val lastEven = listOf(1, 4, 6).asSequence().findLast { x -> x % 2 == 0 }
```
{% endcut %}

- В случае нескольких операций рекомендуется использовать ```asSequence()```:
{% cut "example" %}
```
class Entity(val id: Long, val name: String)

// use this
val ids = listOf(Entity(1, "name"), Entity(2, "имя")).asSequence()
        .filter { e -> e.name.startsWith("n") }
        .map { e -> e.id }
        .toSet()
// instead of this
val ids = listOf(Entity(1, "name"), Entity(2, "имя"))
        .filter { e -> e.name.startsWith("n") }
        .map { e -> e.id }
        .toSet()
```
{% endcut %}

- Не рекомендуется использовать в котлин-коде java-стримы вместо ```Sequence```. Java-стримы лучше оптимизированы, 
однако в случае, если производительность обработки коллекций становится проблемой, 
то лучше не использовать ни стримы ни ```Sequence```.