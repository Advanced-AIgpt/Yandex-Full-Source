# Обработка ошибок

## Иерархия
Некоторые ситуации мы хотим явно обрабатывать -- возвращать больше инфы клиентам, писать метаинфу и т.д.

Для этого существует иерархия исключений, унаследованных от `AbstractHTTPException`. Они отдельно обрабатываются в `HTTPExceptionHandler`'е.

## Сообщения об ошибках

В ответ на запрос, в котором выбрасывается такое исключение, приходит `json` такого вида:
```json
{
  "message": "<human-readable message>",
  "id": "<a UUID we can grep in logs>",
  "time": "<exact timestamp when exception occured>"
}
```

Дополнительное поле `id` пишется в лог вместе с дополнительной инфой, по которому разработчики могут найти детали:
```
2018-06-29 14:04:36.062  WARN 9825 --- [o-auto-1-exec-1] r.y.q.b.exception.HTTPExceptionHandler   : 400/BadRequestException/98ebf5e8-da71-4cf7-b46c-3df2b1f9dd36 at GET /billing/getContentMetaInfo: Unsupported type film
Filtered trace:
    ru.yandex.quasar.billing.controller.BillingControllerTest$Config$TestContentProvider.getContentMetaInfo(BillingControllerTest.java:449)
    ru.yandex.quasar.billing.controller.BillingController.getContentMetaInfo(BillingController.java:255)
    ru.yandex.quasar.billing.filter.YTLoggingFilter.doFilter(YTLoggingFilter.java:52)
    ru.yandex.quasar.billing.filter.StatsLoggingFilter.doFilter(StatsLoggingFilter.java:38)
    ru.yandex.quasar.billing.filter.HeaderModifierFilter.doFilter(HeaderModifierFilter.java:59)
```

Код ответа в этом HTTP-запросе будет максимально точно указывать на причину -- неправильно составленный запрос, ошибка партнёра, етс.

Возможность перезапросов определяется по методу и коду ответа согласно спецификации HTTP.

