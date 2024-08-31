# Объекты авторизации: Auth, LiteAuth и SecureAuth

## Общее описание {#definition}

Объекты _Auth_, _LiteAuth_ и _SecureAuth_ являются базовыми объектами XScript, содержащими информацию об авторизации пользователя.

_Auth_ содержит сведения об авторизации пользователя с точки зрения Яндекс.Паспорта ([http://passport.yandex.ru/](http://passport.yandex.ru/)). Эта авторизация налагает минимальные требования на пароль, передаётся открыто по HTTP. Auth заполняется только для обычных пользователей (не Lite). _LiteAuth_ содержит такие же сведения, что и Auth, но заполняется как для обычных, так и для [Lite-пользователей](https://docs.yandex-team.ru/authdevguide/concepts/LiteAuth_About).

_SecureAuth_ содержит сведения об авторизации пользователя с точки зрения платёжной системы ([https://sauth.yandex.ru/](https://sauth.yandex.ru/)). SecureAuth используется в наиболее защищённых сервисах, Яндекс.Деньги, Яндекс.Баланс и Яндекс.Паспорт, передаётся по HTTPS, налагает серьёзные требования безопасности.

Струкутры _AuthInfo_ и _LiteAuthInfo_ представляют собой облегчённый вид Auth и LiteAuth соответственно и имеют одинаковую структуру, описанную ниже. Как и Auth, AuthInfo заполняется только для обычных пользователей, LiteAuthInfo - и для обычных, и для Lite-пользователей.

```
struct AuthInfo {
    tID uid;
    boolean isSecure;
    string login;
};
```

Класс Yandex::Auth, от которого порождаются все описанные объекты, описан в [auth.idl](https://svn.yandex.ru/wsvn/auth/trunk/idl/auth.idl).


## Особенности работы {#workdetails}

При передаче методу не CORBA-объекта Auth, SecureAuth и LiteAuth возвращают UID пользователя, а если объекты не заполнены, то возвращается 0, даже если явно [приведение типов](parameters-matching-ov.md) не задано.


## Особенности использования {#usagedetails}

Доступ к объектам Auth, LiteAuth и SecureAuth осуществляется через параметры [объектного типа](parameters-complex-ov.md) Auth, LiteAuth и SecureAuth или через структуры AuthInfo и LiteAuthInfo.

Кроме того, получить информацию о логине и UID-е пользователя можно через [параметры-адаптеры](parameters-matching-ov.md) Login, UID, LiteLogin и LiteUID, при чем параметры Login и UID возвращают значимую информацию только для обычных пользователей, а для Lite-пользователей они возвращают пустую строку и 0 соответственно.


## Время жизни {#lifetime}

Объекты Auth, LiteAuth и SecureAuth создаются при соблюдении определённых условий во время обработки запроса на [стадии инициализации](request-handling-init.md).

_Auth_ и _LiteAuth_ создаются при выполнении следующих условий: **\(** XML-файл содержит Auth-параметры (UID, Login, LiteUID, LiteLogin, Auth, LiteAuth, SecureAuth, AuthInfo или LiteAuthInfo) **или** атрибуту need-auth или force-auth, или force-secure-auth тега \<xscript\> присвоено значение "yes" **\) и** кука _Session_id_ установлена и валидна.

Если объявлен атрибут `force-auth="yes"` тега [\<xscript\>](../reference/xscript.md) , но пользователь не авторизован, объект не создаётся, обработка блока прерывается, и пользователь перенаправляется в службу Яндекс.Паспорт с последующим возвратом на исходную страницу. При этом теряется информация о параметрах HTTP-запроса.

_SecureAuth_ создается при выполнении следующих условий: **\(** XML-файл содержит параметры SecureAuth или SecureAuthInfo **или** атрибуту force-secure-auth тега \<xscript\> присвоено значение "yes" **\) и** кука _Secure_session_id_ установлена и валидна **и** запрос пришел по HTTPS.

Если объявлен атрибут `force-secure-auth="yes"` тега [\<xscript\>](../reference/xscript.md), но пользователь не прошел защищенную авторизацию, он перенаправляется на sauth.yandex.ru c последующим возвратом на исходную страницу.

Все описанные объекты удаляются после обработки запроса.


## Пример использования {#example}

Следующий CORBA-блок демонстрирует полную проверку авторизации пользователя, методу передаётся информация по обеим системам авторизации:

```
<block>
   <name>Yandex/Money/WebAdapter.id</name>
   <method>checkSecureAuth</method>
   <param type="Request"/>
   <param type="Auth"/>
   <param type="SecureAuth"/>
</block>
```

### Узнайте больше {#learn-more}
* [Все типы параметров методов, вызываемых в XScript-блоках](../appendices/block-param-types.md)
* [Первый этап: инициализация](../concepts/request-handling-init.md)