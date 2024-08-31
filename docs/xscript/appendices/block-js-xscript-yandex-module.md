# Модуль xscript.yandex

Обеспечивает функции генерирования внутренних ссылок в сервисы через [Клик-демон](http://wiki.yandex-team.ru/Clickdaemon) Яндекса, аналогично xslt-функции [yandex-redirect](xslt-functions.md#yandex-redirect).

**Список функций:**

- redirectUnsigned;
- redirectSigned;
- redirectCrypted.

#### `redirectUnsigned`

Возвращает неподписанную ссылку.

**Входные параметры**

- строка внутренних параметров сервиса Клик-демон, которые указываются после "/redir/" и до "*" включительно, например, `dtype=stred/pid=47/cid=1564/`;
- URL, на который производится редирект.

#### `redirectSigned`

Возвращает ссылку подписанную ключем. Список ключей для подписи ссылок должен быть указан в [файле настроек XScript](config.md).

**Входные параметры**

- строка внутренних параметров сервиса Клик-демон, которые указываются после "/redir/" и до "*" включительно, например, `dtype=stred/pid=47/cid=1564/`;
- URL, на который производится редирект;
- номер ключа, которым должна быть подписана ссылка. Параметр не обязательный.

#### `redirectCrypted`

Возвращает ссылку подписанную ключем и зашифрованную. Список ключей для подписи ссылок должен быть указан в [файле настроек XScript](config.md).

**Входные параметры**

- строка внутренних параметров сервиса Клик-демон, которые указываются после "/redir/" и до "*" включительно, например, `dtype=stred/pid=47/cid=1564/`;
- URL, на который производится редирект;
- номер ключа, которым должна быть подписана ссылка. Параметр не обязательный.

#### Пример:

```
<x:js>
    <![CDATA[
        var prefix = 'dtype=stred/pid=47/cid=1564/*'
        var url = 'http://market.yandex.ru/model.xml?hid=91148&modelid=1020588'

        xscript.print( xscript.yandex.redirectUnsigned(prefix, url) )
    ]]>
</x:js>
<x:js>xscript.print( xscript.yandex.redirectSigned(prefix, url) )</x:js>
<x:js>xscript.print( xscript.yandex.redirectCrypted(prefix, url) )</x:js>
```

Результат:

```xml
<js>
    http://clck.yandex.ru/redir/dtype=stred/pid=47/cid=1564/*data=url%3Dhttp%253A%252F%252Fmarket.yandex.ru%252Fmodel.xml%253Fhid%253D91148%2526modelid%253D1020588
</js>
<js>
    http://clck.yandex.ru/redir/dtype=stred/pid=47/cid=1564/*data=url%3Dhttp%253A%252F%252Fmarket.yandex.ru%252Fmodel.xml%253Fhid%253D91148%2526modelid%253D1020588%26ts%3D1334138051%26uid%3D6614883671333967106&sign=f9317dcba6f23ef2aaec6d78ee3be04a&keyno=1
</js>
<js>
    http://clck.yandex.ru/redir/iV0tK4uMf4nm2iTaAJHJhC3IMNtWosmRRmCLhLPuuBw=data=QVyKqSPyGQwwaFPWqjjgNs3E55X%2FdOEiRNqPeFaZwQDaVmy482kbCDhP7172YWff9IRiSIzp7cmOxV0DdP2qPrVgTeKVl%2B02lRplOtCp8OGVFqdn5Xo6jyt3TmHlYBJEogroin67AqKeUIiFNo7Xe9lt48ZKXXhR&b64e=1&sign=f5ebaa4841f9bdee06b868e1ab96e104&keyno=1
</js>
```

### Узнайте больше {#learn-more}
* [JavaScript-блок](../concepts/block-js-ov.md)
* [js](../reference/js.md)