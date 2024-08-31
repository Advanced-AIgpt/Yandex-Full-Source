# root

Содержит описание скрипта, выполняемого в [Local-блоке](../concepts/block-local-ov.md), или тела цикла [While-блока](../concepts/block-while-ov.md).

## Содержит {#contains}

Любые теги, которые может содержать XML-файл на XScript.

## Содержится в {#contained-in}

[local](local.md), [while](while.md).

## Атрибуты {#attrs}

#|
|| Наименование | Описание | Тип и варианты значения | Значение по умолчанию ||
|| name | Имя корневого XML-элемента, которое будет указано в ответе Local-блока. Наряду с самим именем, может быть указано пространство имен корневого XML-элемента. | Строка. | - ||
|#

## Пример {#example}

```
<x:local xmlns:g="http://www.ya.ru">
     <root name="g:page">
         <xscript/>
         <mist method="set_state_string">
             <param type="String">local_name</param>
             <param type="LocalArg">name</param>
         </mist>
         <mist method="dumpState"/>
     </root>
</x:local>
```

### Узнайте больше {#learn-more}
* [Local-блок](../concepts/block-local-ov.md)
* [While-блок](../concepts/block-while-ov.md)
