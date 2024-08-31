# nameref

Содержит имя переменной в контейнере [State](../concepts/state-ov.md), содержащей имя CORBA-серванта, к которому выполняется запрос.

## Содержит {#contains}

Данный тег не может содержать других тегов.

## Содержится в {#contained-in}

[block](block.md)

## Атрибуты {#attrs}

Нет.

## Пример {#example}

```
<block threaded="yes">
    <nameref>objname</nameref>
    <method>getSomeValue</method>
    <param type="String">1</param>
    <param type="QueryArg">req_param</param>
</block>
```

### Узнайте больше {#learn-more}
* [CORBA-блок](../concepts/block-corba-ov.md)