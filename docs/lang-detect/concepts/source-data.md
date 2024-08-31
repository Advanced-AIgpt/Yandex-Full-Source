# Подготовка исходных данных

## Язык пользователя из Паспорта {#passport}

Чтобы получить настройку языка пользователя из Паспорта, необходимо воспользоваться его API. С помощью метода [userinfo](https://doc.yandex-team.ru/blackbox/reference/MethodUserInfo.dita) следует сделать запрос вида:

```
http://blackbox.yandex.net/blackbox ?
method=userinfo
& uid=11807402
& userip=12.12.12.12
& dbfields=userinfo.lang.uid
```

Значение запрошенного параметра будет находиться в теге `<dbfield>` ответа.

