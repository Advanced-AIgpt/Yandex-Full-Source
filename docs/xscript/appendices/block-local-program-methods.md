# Методы блока Local-program

**Список методов блока**:
- [check](block-local-program-methods.md#check).

#### check {#check}

Определяет, входит ли IP-адрес в пул адресов сетей, подключенных к программе <q>Локальная сеть</q>. Возвращает [код географического региона](http://geoadmin.yandex.ru), к которому относится IP-адрес.

**Входные параметры**:

- IP-адрес (необязательно). Если не задан, используется значение переменной [`remote_ip`](protocol-arg.md#remote_ip) типа [ProtocolArg](protocol-arg.md). В настоящий момент поддерживаются IP-адреса, соответствующие [спецификации IPv4](http://ru.wikipedia.org/wiki/IP#.D0.92.D0.B5.D1.80.D1.81.D0.B8.D1.8F_4_.28IPv4.29).
    
    В метод может быть передан только один параметр.

**Примеры использования**

_Блок:_

```
<x:local-program method="check">
    <param type="String">217.195.64.1</param>
</x:local-program> 
```

_Вывод:_

```
<result region="2" code="1" status="OK">regional program user</result>
```

- `region="2"`: код региона 2, [соответствует Санкт-Петербургу](http://geoadmin.yandex.ru/#region:2);
- `code="1"`: код ответа 1, IP-адрес принадлежит к пулу адресов сетей, входящих в программу <q>Локальная сеть</q>;
- `status="OK"`: IP-адрес соответствует спецификации IPv4;
- `regional program user`: текстовая интерпретация кода ответа 1.

_Блок:_

```
<x:local-program method="check">
    <param type="String">217.197.232.1</param>
</x:local-program> 
```

_Вывод:_

```
<result region="2" code="2" status="OK">not in our regional program</result>
```

- `region="2"`: код региона 2, [соответствует Санкт-Петербургу](http://geoadmin.yandex.ru/#region:2);
- `code="2"`: код ответа 2, IP-адрес НЕ принадлежит к пулу адресов сетей, входящих в программу <q>Локальная сеть</q>;
- `status="OK"`: IP-адрес соответствует спецификации IPv4;
- `not in our regional program`: текстовая интерпретация кода ответа 2.

_Блок:_

```
<x:local-program method="check">
    <param type="String">62.207.137.130</param>
</x:local-program> 
```

_Вывод:_

```
<result region="79" code="4" status="OK">no regional peers</result>
```

- `region="79"`: код региона 79, [соответствует Магадану](http://geoadmin.yandex.ru/#region:79);
- `code="4"`: код ответа 4, в данном регионе (Магадане) нет ни одного провайдера, участвующего в программе <q>Локальная сеть</q>;
- `status="OK"`: IP-адрес соответствует спецификации IPv4;
- `no regional peers`: текстовая интерпретация кода ответа 4.

_Блок:_

```
<x:local-program method="check">
    <param type="String">192.168.1.1</param>
</x:local-program> 
```

_Вывод:_

```
<result code="4" status="OK">no regional peers</result>
```

Несмотря на то, что IP-адрес соответствует спецификации IPv4, он не может быть использован для определения местоположения пользователя.

_Блок:_

```
<x:local-program method="check">
    <param type="String">91.263.168.1</param>
</x:local-program>
```

_Вывод:_

```
<result status="ERROR">INVALID IP</result>
```
 IP-адрес не соответствует спецификации IPv4.

_Блок:_

```
<x:local-program method="check"/>
```

_Вывод (при обращении из московского офиса Яндекса):_

```
<result region="9999" code="4" status="OK">no regional peers</result>
```

Метод `check` вызывается берез передачи параметра. В качестве IP-адреса используется значение переменной [`remote_ip`](protocol-arg.md#remote_ip) типа [ProtocolArg](protocol-arg.md).

Код региона 9999 соответствует [сети московского офиса Яндекса](http://geoadmin.yandex.ru/#region:9999). Считается, что офисы Яндекса не подключены к программе <q>Локальная сеть</q>.

### Узнайте больше {#learn-more}
* [Local-program-блок](../concepts/block-local-program-ov.md)
* [local-program](../reference/local-program.md)
