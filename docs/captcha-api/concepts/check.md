# Метод check {#check}

Проверяет ответ пользователя.

Ответ должен быть проверен в течение тайм-аута (60 минут от момента вызова метода [generate](generate.md)).

## Синтаксис запроса {#request}

```
http://api.captcha.yandex.net/check
  ? type=<std|lite|rus>
  & rep=<user_input>
  & key=<captcha_key>
```
#|
|| **Параметр** | **Значение** ||
|| type* |  Тип капчи:
 - std — обычная.
 - lite — упрощенная.
 - rus — из букв русского алфавита.

Значение аргумента должно совпадать с указанным при вызове метода [generate](generate.md). ||
|| rep* | Ответ пользователя для проверки. ||
|| key* | Ключ капчи, полученный при вызове метода [generate](generate.md). ||
|#

*Обязательный параметр

> ## Пример
> `http://api.captcha.yandex.net/check?type=std&rep=539270&key=10pIGk1YA_uMYERwCg9Zzltn_cQ3bBOF`

## Формат ответа {#answer}

Ответом является XML-элемент `image_check`, содержащий результат проверки.

1. Ответ при правильном вводе символов с изображения:
    ```xml
    <?xml version='1.0'?>
    <image_check>ok</image_check>
    ```
    
1. Ответ при ошибочном вводе символов с изображения:
    ```xml
    <?xml version='1.0'?>
    <image_check>failed</image_check>
    ```
    
1. Ответ при неверном ключе капчи или если истек тайм-аут теста:
    ```xml
    <?xml version='1.0'?>
    <image_check error="not found">failed</image_check>
    ```
    
1. Ответ при несовпадении аргумента `type` с указанным в методе [generate](generate.md):
    ```xml
    <?xml version='1.0'?>
    <image_check error="inconsistent type">failed</image_check>
    ```
    

## Сообщения об ошибках {#check}

Метод возвращает сообщения о стандартных HTTP-ошибках:

- 403 — отсутствуют полномочия на доступ;
- 500 — ошибка в работе капча-сервера, причина ошибки не уточняется;
- 502 — капча-сервер не обработал запрос (не запущен или высокая нагрузка).

