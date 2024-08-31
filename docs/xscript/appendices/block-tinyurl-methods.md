# Методы Tinyurl-блока

**Список методов блока**:
- [retrieve](block-tinyurl-methods.md#retrieve);
- [store](block-tinyurl-methods.md#store).

#### `retrieve` {#retrieve}

Возвращает длинный URL по токену.

В случае неверного формата токена блок возвращает следующий ответ:

```
<tinyurl error="BAD_TOKEN"/>
```

Если токен не найден:

```
<tinyurl error="NOT_FOUND"/>
```

**Входные параметры**:

- токен;
- (опциональный параметр) имя проекта.

**Пример**:

```
<x:tinyurl method="retrieve">
     <param type="String">CBR5xA</param>
     <param type="String">project_name</param>
</x:tinyurl>
```

В результате данного вызова будет сформирован следующий ответ:

```
<tinyurl>
     <url>http://www.example.ru/example?arg1=1&amp;arg2=2</url>
</tinyurl>
```

#### `store` {#store}

Сохраняет длинный URL и возвращает для него короткий токен.

**Входные параметры**:

- длинный URL. Полный URL указывать не обязательно - можно передать только часть после имени хоста;
- (опциональный параметр) имя проекта.

**Пример**:

```
<x:tinyurl method="store">
     <param
type="String">http://www.example.ru/example?arg1=1&amp;arg2=2</param>
     <param type="String">project_name</param>
</x:tinyurl>
```

В результате данного вызова будет сформирован следующий ответ:

```
<tinyurl>
     <token>CBR5xA</token>
</tinyurl>
```

### Узнайте больше {#learn-more}
* [Tinyurl-блок](../concepts/block-tinyurl-ov.md)
* [tinyurl](../reference/tinyurl.md)
* ["Яндексовский" сервис для укорачивания URL-ов](http://wiki.yandex-team.ru/tinyurl)