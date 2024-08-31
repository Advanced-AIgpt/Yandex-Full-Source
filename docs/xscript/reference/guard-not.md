# guard-not

[Guard](guard.md)-отрицание.

Определяет условие при выполнении которого обработка блока будет прервана. Блок может содержать несколько тегов `<guard>` и [`guard-not`](guard-not.md) и обрабатывается полностью при выполнении всех условий guard и невыполнении всех условий guard-not.

Условие задается так же как и в теге [`guard`](guard.md), но работает <q>наоборот</q>, то есть выполнение условия приведет к прерыванию процесса обработки блока.

## Содержит {#contains}:

Данный тег не может содержать других тегов.

## Содержится в {#contained-in}

тегах любых [блоков](../concepts/block-ov.md).

## Атрибуты {#attrs}
#|
|| Наименование | Описание | Тип и варианты значения | Значение по умолчанию ||
|| type | Опциональный атрибут.

Тип переменной, значение которой проверяется в `<guard>`.

Если данный атрибут отсутствует, считается, что переменной находится в контейнере State. | Строка. Возможные значения: QueryArg, StateArg, Cookie, HttpHeader, UID, LiteUID, Login и LiteLogin. | - ||
|| value | Опциональный атрибут.

Значение для сравнения.

Если значение переменной НЕ совпадает со значением данного атрибута, блок будет выполнен. | Любой тип | - ||
|#

## Пример {#example}

```
<mist>
     <guard>var1</guard>
     <guard>var2</guard>
     <guard-not>var3</guard-not>
     <method>set_state_string</method>
     <param type="String">var</param>
     <param type="String">sss</param>
</mist>
```

### Узнайте больше {#learn-more}
* [Атрибут guard-not](../appendices/attrs-ov.html#attrs-ov__guard-not)
* [guard](../reference/guard.md)
* [Общий процесс обработки запроса](../concepts/request-handling-ov.md)
* [Приводимые типы параметров](../concepts/parameters-matching-ov.md)