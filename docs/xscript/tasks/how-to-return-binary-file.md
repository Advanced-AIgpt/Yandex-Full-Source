# Как вернуть пользователю двоичный файл

Для того чтобы вернуть пользователю двоичный файл, необходимо выполнить следующие действия:

1. В коде XML-страницы описать запрос бинарного файла одним из следующих способов:

	* Один из CORBA-компонентов, вызываемых из CORBA-блоков, должен сохранить файл в объекте [Request](../concepts/request-ov.md), вызвав его метод `Write` или `WriteFile`. Соответствующий метод должен включать параметр типа Request.

	* Для получения бинарного контента по HTTP используется метод [get_binary_page](../appendices/block-http-methods.md#get_binary_page) HTTP-блока .

	* Для получения бинарного контента из файла используется метод [loadBinary](../appendices/block-file-methods.md#loadbinary) File-блока.
    
1. Выставить подходящий заголовок `Content-Type` в заголовке HTTP-ответа. Например, так:
    
```
<page> 

    <xscript> 
       <add-headers> 
          <header name="Content-type" value="application/pdf"/> 
       </add-headers> 
    </xscript> 

    <block> 
       <name>Yandex/Project/Shop/Yashop.id</name> 
       <method>addOrder</method> 
       <param type="Auth" /> 
       <param type="Request" /> 
    </block> 
 

</page>
```

В результате выполнения процедуры при загрузке данного URL пользователю вместо HTML-страницы браузер предложит сохранить бинарный файл, подготовленный методом addOrder компонента `Yandex/Project/Shop`.

### Узнайте больше {#learn-more}
* [Общий процесс обработки запроса](../concepts/request-handling-ov.md)
* [Как выставить заголовки HTTP-ответа и куки](../tasks/how-to-set-headers.md)
* [CORBA-блок](../concepts/block-corba-ov.md)