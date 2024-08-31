# Методы Auth-блока

Многие методы Auth-блока выполняют получение информации о пользователе на основании UID. UID может передаваться методу в виде параметра типа LongLong, UID, LiteUID, SecureUID, Auth, LiteAuth или SecureAuth.

**Список методов Auth-блока**:
- [get_all_info](block-auth-methods.md#get_all_info);
- [get_all_info_by_service](block-auth-methods.md#get_all_info_by_service);
- [get_bulk_fields](block-auth-methods.md#get_bulk_fields);
- [get_bulk_logins](block-auth-methods.md#get_bulk_logins);
- [get_bulk_names](block-auth-methods.md#get_bulk_names);
- [get_fields](block-auth-methods.md#get_fields);
- [get_subscription_info](block-auth-methods.md#get_subscription_info);
- [set_state_by_auth](block-auth-methods.md#set_state_by_auth);
- [set_state_by_login](block-auth-methods.md#set_state_by_login);
- [set_state_by_service](block-auth-methods.md#set_state_by_service);
- [set_state_by_session](block-auth-methods.md#set_state_by_session);
- [set_state_by_suid](block-auth-methods.md#set_state_by_suid);
- [set_state_by_uid](block-auth-methods.md#set_state_by_uid);
- [set_state_email_by_domain](block-auth-methods.md#set_state_email_by_domain);
- [set_state_login](block-auth-methods.md#set_state_login);
- [set_state_mail_db](block-auth-methods.md#set_state_mail_db);
- [set_state_username](block-auth-methods.md#set_state_username).

#### `get_all_info` (`getAllInfo`) {#get_all_info}

Получает полную информацию об учетной записи пользователя по его UID.

Списки полей и SID-ов, которые могут быть запрошены, определены в конфигурационном файле XScript с помощью параметров [account-info-fields](config-params.md#account-info-fields) и [sids](config-params.md#sids).

**Входные параметры**: UID пользователя.

**Примеры использования**:
1. ```
    <auth-block>
    <method>getAllInfo</method>
    <param type="Long">1234</param>
    </auth-block>
    ```
    
1. ```
    <auth-block>
    <method>getAllInfo</method>
    <param type="UID"/>
    </auth-block>
    ```

#### `get_all_info_by_service` (`getAllInfoByService`) {#get_all_info_by_service}

Возвращает полную информацию об учетной записи пользователя, если он подписан на сервис с указанным id.

**Входные параметры**:
- UID пользователя;
- id сервиса.

**Примеры использования**:
1. ```
    <auth-block>
    <method>getAllInfoByService</method>
    <param type="Long">1234</param>
    <param type="Long">7</param>
    </auth-block>
    ```

1. ```
    <auth-block>
    <method>getAllInfoByService</method>
    <param type="UID"/>
    <param type="Long">7</param>
    </auth-block>
    ```

#### `get_bulk_fields` (`getBulkFields`) {#get_bulk_fields} 

Возвращает информацию о пользователях из таблиц [acount info и subscription](https://docs.yandex-team.ru/authdevguide/concepts/DB_About) сервиса Яндекс.Паспорт.

Для получения информации о каждом пользователе используется метод [get_fields](block-auth-methods.md#get_fields).

**Входные параметры**:
- список UID пользователей;
- второй и третий параметры совпадают с соответствующими параметрами метода [get_fields](block-auth-methods.md#get_fields).

**Пример использования**:

```
<auth-block>
     <method>getBulkLogins</method>
     <param type="String">12,13,2323423</param>
</auth-block>
```

#### `get_bulk_logins` (`getBulkLogins`) {#get_bulk_logins} 

Возвращает список логинов для списка UID-ов.

**Входные параметры**: UID-ы, перечисленные через запятую.

**Пример использования**:

```
<auth-block>
     <method>getBulkLogins</method>
     <param type="String">12,13,2323423</param>
</auth-block>
```

#### `get_bulk_names` (`getBulkNames`) {#get_bulk_names}

Возвращает список username-ов для списка UID-ов.

Username-ом считается содержимое первого непустого поля из следующего списка полей БД Яндекс.Паспорта: account_info.nickname, account_info.fio и accounts.login.

**Входные параметры**: UID-ы, перечисленные через запятую.

**Пример использования**:

```
<auth-block>
     <method>getBulkNames</method>
     <param type="String">12,13,2323423</param>
</auth-block>
```

#### `get_fields` (`getFields)` {#get_fields}

Возвращает информацию о пользователе из таблиц [acount info и subscription](https://docs.yandex-team.ru/authdevguide/concepts/DB_About) сервиса Яндекс.Паспорт.

Для получения информации из таблицы subscription необходимо заказать гранты на доступ к полям suid, login и login_rule с указанием идентификатора проекта ([SID](http://wiki.yandex-team.ru/passport/sids)-а), по которому будут запрашиваться данные.

При отсутствии прав на доступ хотя бы к одному из запрашиваемых полей будет выдано сообщение об ошибке [xscript_invoke_failed](../concepts/error-diag-ov.md).

Рекомендуется по возможности использовать данный метод вместо метода [get_all_info](block-auth-methods.md#get_all_info), так как он позволяет делать более "лёгкие" запросы к БД, что снижает нагрузку на Blackbox.

**Входные параметры**:
- UID пользователя;
- список запрашиваемых полей таблицы account_info, а также следующие данные о пользователе:
    - karma - информация о карме пользователя;
    - karmastatus - статус кармы пользователя;
    - bantime - время в Unix-формате, после которого учетная запись пользователя официально считается "спамовой";
    - regname - имя, введенное пользователем при регистрации (может отличаться от логина);
    - displayname - отображаемое имя пользователя ([display_name](http://wiki.yandex-team.ru/passport/social/about)) из Чёрного ящика;
    - social_provider - двухсимвольный код провайдера социального профиля (например, <q>tw</q>- twitter);
    - social_profile_id - идентификатор социального профиля;
    - yandex_login - портальный логин пользователя. Не заполняется для социальных аккаунтов. Пустой yandex_login можно использовать как признак социального пользователя.
    - social_redirect_target - токен для формирования [редиректа на социальный профиль](http://wiki.yandex-team.ru/social/redirect).
    
- список запрашиваемых [SID](http://wiki.yandex-team.ru/passport/sids)-ов из таблицы subscription. Необязательный параметр.

**Пример использования**:

```
<auth-block>
     <method>getFields</method>
     <param type="UID"/>
     <param type="String">fio,sex,nickname,reg_date,city,country, karma, regname</param>
     <param type="String">2,3,4,5,44</param> 
</auth-block>
```

В результате выполнения блока будет возвращен следующий XML-фрагмент:

```
<user>
     <city/>
     <authid>1327314777183:1600947985:2</authid>
     <country>notselected</country>
     <fio>Vasily Pupkin</fio>
     <karma>0</karma>
     <lite_user>0</lite_user>
     <login>vasya</login>
     <need_reset>0</need_reset>
     <nickname>Vasya</nickname>
     <regname>vasya</regname>
     <reg_date>2000-00-10 10:00:00</reg_date>
     <sex>1</sex>
     <uid>11111111</uid>
     <services>
         <service id="2" suid="65159986" login="vasya" login_rule="1"/>
         <service id="3" suid="67497944" login="vasya" login_rule="1"/>
         <service id="4" suid="146667920" login="vasya" login_rule="1"/>
         <service id="5" suid="65348287" login="vasya" login_rule="1"/>
         <service id="44" suid="95546613" login="vasya" login_rule="1"/>
     </services>
</user>
```

#### `get_subscription_info` (`getSubscriptionInfo`) {#get_subscription_info}

Получает информацию о подписках пользователя на сервисы по его _UID_.

**Входные параметры**: параметр, на основании которого происходит получение информации о подписках.

**Примеры использования**:
1. ```
    <auth-block>
    <method>getSubscriptionInfo</method>
    <param type="Long">1234</param>
    </auth-block>
    ```
    
1. ```
    <auth-block>
    <method>getSubscriptionInfo</method>
    <param type="UID"/>
    </auth-block>
    ```

####  `set_state_by_auth` (`setStateByAuth`) {#set_state_by_auth}

Получает логин пользователя по его UID и добавляет логин и UID в контейнер [State](../concepts/state-ov.md).

**Входные параметры**: 
- имя переменной в State типа LongLong, которой будет присвоено значение UID-а, например, var. Логин записывается в переменную с таким же именем и постфиксом '_login', т.е. для указанного выше примера это будет переменная 'var_login';
- UID пользователя.

**Пример использования**:
```
<auth-block>
    <method>set_state_by_auth</method>
    <param type="String">var</param>
    <param type="UID"/>
</auth-block>
```

#### `set_state_by_login` (`setStateByLogin`) {#set_state_by_login}

Получает UID по логину пользователя и добавляет его в контейнер State.

**Входные параметры**: 
- имя переменной в State типа LongLong, которой будет присвоено значение UID-а, например, var;
- логин пользователя.

**Пример использования**:
```
<auth-block>
   <method>set_state_by_login</method>
   <param type="String">var</param>
   <param type="QueryArg">login</param>
</auth-block>
```

#### `set_state_by_service` (`setStateByService`) {#set_state_by_service}

Получает  и логин пользователя по UID и SID добавляет их в контейнер State.

**Входные параметры**: 
- имя переменной в State типа LongLong, которой будет присвоено значение SUID-а, например, var. Логин записывается в переменную с таким же именем и постфиксом '_login', т.е. для указанного выше примера это будет переменная 'var_login';
- UID пользователя;
- SID.

**Пример использования**:
```
<auth-block>
    <method>set_state_by_service</method>
    <param type="String">var</param>
    <param type="UID"/>
    <param type="QueryArg">sid</param>
</auth-block>
```

#### `set_state_by_session` (`setStateBySession`) {#set_state_by_session}

Получает UID по значению куки Session_id, хосту и IP пользователя. Добавляет в контейнер State переменную с именем var, типом LongLong и значением UID.

**Входные параметры**:
- имя переменной в State типа LongLong, которой будет присвоено значение UID-а, например, var;
- Session_id;
- хост;
- IP-адрес пользователя.

**Пример использования**:
```
<auth-block>
   <method>set_state_by_session</method>
   <param type="String">var</param>
   <param type="QueryArg">session_id</param>
   <param type="QueryArg">хост</param>
   <param type="QueryArg">IP-адрес пользователя</param>
</auth-block>
```
:

#### `set_state_by_suid` (`setStateBySUID`) {#set_state_by_suid}

Получает UID по SUID и SID и добавляет его в контейнер State.

**Входные параметры**:
- имя переменной в State типа LongLong, которой будет присвоено значение UID-а, например, var. Логин записывается в переменную с таким же именем и постфиксом '_login', т.е. для указанного выше примера это будет переменная 'var_login';
- SUID;
- SID.

**Пример использования**:
```
<auth-block>
   <method>set_state_by_suid</method>
   <param type="String">var</param>
   <param type="QueryArg">suid</param>
   <param type="QueryArg">sid</param>
</auth-block>
```

#### `set_state_by_uid` (`setStateByUID`) {#set_state_by_uid}

См. [set_state_by_service](block-auth-methods.md#set_state_by_service).

#### `set_state_email_by_domain` (`setStateEmailByDomain`) {#set_state_email_by_domain}

Получает email пользователя для заданного UID и домена и добавляет его в контейнер State.

Email пользователя конструируется из логина и домена (логин@домен). Домен подается методу на вход. Если домен отличается от "narod.ru", логином считается логин подписки c SID 2. Если домен - "narod.ru", то при наличии подписки c SID 16 используется ее логин, иначе - логин подписки с SID 2.

См. [Список SID-ов Паспорта](https://wiki.yandex-team.ru/passport/sids).

**Входные параметры**:

- имя переменной в State, которой будет присвоено значение email;
- UID пользователя;
- домен. Значение по умолчанию: "yandex.ru".

**Пример использования**:

```
<auth-block>
      <method>set_state_email_by_domain</method>
      <param type="String">var</param>
      <param type="UID"/>
      <param type="String">narod.ru</param> 
</auth-block>
```

#### `set_state_login` (`setStateLogin`) {#set_state_login}

Получает логин по UID и добавляет его в контейнер State.

**Входные параметры**:
- имя переменной в State типа LongLong, которой будет присвоено значение логина;
- UID пользователя.

**Пример использования**:
```
<auth-block>
   <method>set_state_login</method>
   <param type="String">var</param>
   <param type="QueryArg">uid</param>
</auth-block>
```

#### `set_state_mail_db` (`setStateMailDB`) {#set_state_mail_db}

По UID и SID получает значение из поля db_id в таблице `hosts` БД Паспорта и добавляет его в контейнер State.

**Входные параметры**:

- имя переменной в State, которой будет присвоено значение из поля db_id;
- UID пользователя;
- SID. Допустимые значения: 2 и 4. См. [Список SID-ов Паспорта](https://wiki.yandex-team.ru/passport/sids).

**Пример использования**:

```
<auth-block>
      <method>set_state_mail_db</method>
      <param type="String">var</param>
      <param type="UID"/>
      <param type="Long">2</param>
</auth-block>
```

#### `set_state_username` (`setStateUsername`) {#set_state_username} 

Получает username пользователя по UID и добавляет его в контейнер State.

Username-ом считается содержимое первого непустого поля из следующего списка полей БД Яндекс.Паспорта: account_info.nickname, account_info.fio и accounts.login.

**Входные параметры**:

- имя переменной в State, которой будет присвоено значение username;
- UID пользователя.

**Пример использования**:

```
<auth-block>
     <method>set_state_username</method>
     <param type="String">var</param>
     <param type="UID"/>
</auth-block>
```

### Узнайте больше {#learn-more}
* [Auth-блок](../concepts/block-auth-ov.md)
* [auth-block](../reference/auth-block.md)