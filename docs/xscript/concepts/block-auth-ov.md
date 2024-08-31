# Auth-блок

_Auth-блок_ предназначен для работы с базой авторизационных данных, обращение к которой происходит через [Blackbox](https://wiki.yandex-team.ru/passport/blackbox).

Содержит методы для определения UID, логина, SUID, а также дополнительной информации о пользователе.

Auth-блок может выполняться асинхронно (если установлен атрибут [threaded="yes"](../appendices/attrs-ov.md#threaded)).

**Пример Auth-блока**:

```
<auth-block>
   <method>set_state_by_auth</method>
   <param type="String">var</param>
   <param type="Auth"/>
</auth-block>
```

В приведенном примере выполняется получение UID пользователя на основании [параметра Auth](auth-ov.md).

Статус ответа Blackbox-а можно получить, установив атрибут [blackbox-status](../reference/xscript.md#blackbox-status) в теге \<xscript\>.

### Узнайте больше {#learn-more}
* [Методы Auth-блока](../appendices/block-auth-methods.md)
* [auth-block](../reference/auth-block.md)
