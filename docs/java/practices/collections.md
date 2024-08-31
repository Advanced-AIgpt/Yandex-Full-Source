### Ссылки на коллекции через интерфейс или реализацию?

В общем случае рекомендуется ссылаться на любые объекты из _Java Collection Framework_ через интерфейсы, 
которые обеспечивают минимально необходимый набор методов.

В частности для ссылок на коллекции используйте интерфейс ```java.util.Collection```. Если для коллекции важен порядок 
элементов или их уникальность, то используйте ```java.util.List``` или ```java.util.Set``` соответственно. 

Эта рекомендация касается ссылок на коллекции во всех местах (в сигнатуре методов, полях класса, локальных переменных).

Если интерфейсов недостаточно для эффективной реализации бизнес-логики, то используйте ссылки на реализации.

{% cut "Мотивация" %}

Код, написанный со ссылками на интерфейсы, в большей степени проявляет такие качества как:

1. Гибкость (код легче изменять). Например, избавляемся от лишних конвертаций коллекций перед вызовом метода.
2. Единообразность/шаблонность (код легче читать). Код проще воспринимать, когда в нём используются знакомые интерфейсы
с более простой структурой, чем у реализаций.
3. Понятность задумки автора (код легче читать). Например, при использовании интерфейсов ```java.util.List``` или 
```java.util.Set``` в коде, в котором явно не требуется ни порядок, ни уникальность элементов и нет комментариев,
возникают лишние вопросы: есть ли скрытый смысл в их использовании, хотел ли автор этим что-то сказать?
4. Устойчивость к ошибкам. Со ссылками через интерфейс с минимально необходимым набором методов, 
сложнее случайным образом нарушить неявные предположения, например, относительно внутреннего состояния объекта.

Полезные ссылки:
- Effective Java (3rd edition): "Item 64: Refer to objects by their interfaces"

{% endcut %}

### Использовать ли ссылки на немодифицируемые коллекции?

Не используйте в качестве ссылок на коллекции интерфейсы или классы, которые отличаются от стандартных только тем,
что они немодифицируемые.

Используйте например ```java.util.Collection``` вместо 
- ```com.google.common.collect.ImmutableCollection```
- ```org.apache.commons.collections.collection.UnmodifiableCollection```
- или самописного класса.

Факт немодифицируемости возвращаемой коллекции можно указать в публичных классах и интерфейсах с _javadoc_, 
например, при помощи тега ```@implNote```:

```(java)
/**
 * @return list of services without instances
 * @implNote returned collection is unmodifiable
 */
public Collection<String> getDanglingServices() {
    return Collections.unmodifiableCollection(danglingServices);
}
```

{% cut "Мотивация" %}

1. Описанный подход используется в _Java Collection Framework_. Смешивать несколько подходов не рекомендуется ради единообразия кодовой базы.

2. Явное использование немодифицируемых коллекций может привести к ошибочному предположению о неизменяемости всего объекта,
когда в такую коллекцию собраны изменяемые объекты.

{% endcut %}

### Последовательность элементов как возвращаемое значение метода: Collection, Stream или Iterable?

Если из метода необходимо вернуть последовательность элементов, то по возможности используйте ```java.util.Collection```,
так как он удобным образом предоставляет выбор стиля для обработки результата в клиентском коде (_for-each_ или _stream api_).

В случаях когда невозможно создать объект ```java.util.Collection``` (например, нельзя все элементы загрузить в память),
выбирайте интерфейс в зависимости от того, сколько раз может быть просмотрена последовательность элементов.

Если последовательность элементов может быть просмотрена только один раз, то ```java.util.stream.Stream```, иначе ```java.util.Iterable```.

Данная рекомендация касается публичных методов интерфейсов и классов. В приватных методах стоит использовать то, что удобнее в конкретной ситуации.

{% cut "Мотивация" %}

Предоставление клиентскому коду удобного выбора между _for-each_ и _stream api_ облегчает его изменение и повышает
читабельность (за счёт отсутствия преобразований между ```Stream``` и ```Iterable```).

Полезные ссылки:
- Effective Java (3rd edition): "Item 47: Prefer Collection to Stream as a return type"
- [Почему интерфейс ```Stream``` не расширяет ```Iterable```](http://mail.openjdk.java.net/pipermail/lambda-dev/2013-March/008877.html)

{% endcut %}