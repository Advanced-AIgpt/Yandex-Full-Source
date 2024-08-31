# Модуль xscript.sanitize

**Список функций:**

- sanitizeHTML

#### `sanitizeHTML` {#sanitize}

Использует алгоритм санитайзинга из Аркадии для <q>очистки</q> html. Функция работает аналогично xslt-функции [sanitize](xslt-functions.md#sanitize).

**Входные параметры**:

- Строка с HTML-разметкой;
- Базовый URL, который будет добавляться ко всем относительным ссылкам. Если его не указать то сылка будет вырезана. Необязательный параметр;
- Количество непробельных символов, после которых будет вставляться <wbr />. Необязательный параметр.

#### Пример:

```
<x:js>
    <![CDATA[
        var html = '<div onclick="alert(1)"><a href="/godzillah_warrior">Godzillah Warrior</a></div>'
        xscript.print( xscript.sanitize.sanitizeHTML(html) )
    ]]>
</x:js>

```

Результат:

```xml
<js>
    <div><a>Godzillah Warrior</a></div>
</js>
```

### Узнайте больше {#learn-more}
* [JavaScript-блок](../concepts/block-js-ov.md)
* [js](../reference/js.md)