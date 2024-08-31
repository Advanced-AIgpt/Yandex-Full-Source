# Специальные типы параметров

К специальным типам параметров относятся Hostname и HTTPUser.

_Hostname_ - это тип параметра, содержащий имя машины, на которой работает XScript.

Пример использования Hostname:

```
<mist>
   <method>set_state_string</method>
   <param type="String">h</param>
   <param type="Hostname"/>
</mist>
```

Вместо типа _HTTPUser_ рекомендуется использовать переменную [ProtocolArg](../appendices/protocol-arg.md) `http_user`.

### Узнайте больше {#learn-more}
* [Простые типы параметров](../concepts/parameters-simple-ov.md)
* [Объектные и структурные типы параметров](../concepts/parameters-complex-ov.md)
* [Приводимые типы параметров](../concepts/parameters-matching-ov.md)
