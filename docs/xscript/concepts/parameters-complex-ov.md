# Объектные и структурные типы параметров

Объектные и структурные типы параметров применяются для передачи методам CORBA-компонентов следующих базовых объектов и структур XScript:

- [объектов Auth, LiteAuth, SecureAuth и структур AuthInfo и LiteAuthInfo](auth-ov.md);
- [контейнера State](state-ov.md);
- [объекта Request и структуры RequestData](request-ov.md);
- [объекта CustomMorda](custom-morda-ov.md);
- [структуры Tag](tag-ov.md).

Эти объекты и структуры нельзя передавать за пределы CORBA (за исключением Auth, LiteAuth, SecureAuth, AuthInfo и LiteAuthInfo, которые используются в [Auth-блоке](block-auth-ov.md)), поэтому они сами не передаются по значению.

Если CORBA-компонент, получив объект или структуру, пытается обратиться к несуществующему полю данных, то XScript возвращает ему пустое значение типа Any.

Для передачи отдельных полей данных этих объектов используются [типы-адаптеры](parameters-matching-ov.md).

Примеры передачи параметров объектных и структурных типов:

```
<param type="Auth"/>

<param type="Request"/>

<param type="CustomMorda">

<param type="State"/>

<param type="Tag"/>
```

### Узнайте больше {#learn-more}
* [Приводимые типы параметров](../concepts/parameters-matching-ov.md)
* [Все типы параметров методов, вызываемых в XScript-блоках](../appendices/block-param-types.md)