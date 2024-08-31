# Local-program-блок 

Блок **Local-program** предназначен для определения принадлежности IP-адреса к сети, подключенной к [региональной программе Яндекса (<q>Локальная сеть</q>)](http://local.yandex.ru/).

Использование данного блока позволяет установить, входит ли IP-адрес в пул адресов сетей, подключенных к программе <q>Локальная сеть</q>, и определить код географического региона, к которому относится IP-адрес.

**Пример блока**:

```
<x:local-program method="check">
    <param type="String">217.197.232.1</param>
</x:local-program> 
```

Метод `check` может быть вызван и без явного указания IP-адреса. Если метод `check` вызывается без передачи параметра, используется значение переменной переменной [`remote_ip`](../appendices/protocol-arg.md#remote_ip) типа [ProtocolArg](../appendices/protocol-arg.md).

```
<x:local-program method="check"/>
```

Блок не может быть выполнен асинхронно, результаты его работы не [кэшируются](block-results-caching.md). Использование пространства имён XScript обязательно.

Блок Local-program разработан в качестве замены HTTP-запросов, использующих для определения принадлежности IP-адреса к программе <q>Локальная сеть</q>, [обращения к URL region.narod.yandex.net](http://wiki.yandex-team.ru/DKorolkov/YandexLocal). То есть **не следует использовать код вида**

```
<x:http threaded="yes" method="getHttp">
    <x:param type="String">http://region.narod.yandex.net/?region=auto&ip=...</x:param>
    <x:param type="ProtocolArg">remote_ip</x:param>
</x:http> 
```

Вместо этого необходимо воспользоваться блоком Local-program. Это позволяет избежать обмена данными между фронт- и бак-эндом, значительно увеличить скорость обработки запроса и снизить потребление ресурсов.


### Узнайте больше {#learn-more}
* [Методы блока Local-program](../appendices/block-local-program-methods.md)
* [local-program](../reference/local-program.md)