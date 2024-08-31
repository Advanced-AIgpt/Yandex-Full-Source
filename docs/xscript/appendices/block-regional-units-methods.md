# Методы Regional-units-блока

**Список методов блока**:
- transform;
- transformDateTime.

#### `transform`

Возвращает представление входного значения в выбранной системе единиц.

**Входные параметры**:

- [числовое обозначение системы единиц](https://doc.yandex-team.ru/lib/libregional-units/concepts/regional-units.html#interfaces) или [ISO-код](https://doc.yandex-team.ru/lib/libregional-units/concepts/regional-units.html#interfaces);
- [категория измерения](https://doc.yandex-team.ru/lib/libregional-units/concepts/regional-units.html#interfaces);
- [входное значение](https://doc.yandex-team.ru/lib/libregional-units/concepts/regional-units.html#common-logic).

#### `transformDatetime`

Преобразовывает POSIX-время в определенный пользователем формат.

**Параметры**:

- POSIX-время;
- [локаль](https://doc.yandex-team.ru/lib/libregional-units/concepts/regional-units.html#interfaces);
- [формат результата](https://doc.yandex-team.ru/lib/libregional-units/concepts/regional-units.html#logic-time).

#### Пример:

```xml
<regional-units-block>
    <method>transform</method>
    <param type="String">US</param>
    <param type="String">distance</param>
    <param type="String">1700</param>
</regional-units-block>

<regional-units-block>
    <method>transformDatetime</method>
    <param type="String">1315147857</param>
    <param type="String">ru-RU</param>
    <param type="String">datetime</param>
</regional-units-block>
```

В результате данного вызова будет сформирован следующий ответ:

```xml
<result unit="miles" value="1.054"/>
<result format="datetime" value="воскресенье, 04 января 2011 г. 18:50:57"/>
```

### Узнайте больше {#learn-more}
* [Regional-units-блок](../concepts/block-regional-units-ov.md)
* [regional-units-block](../reference/regional-units.md)
