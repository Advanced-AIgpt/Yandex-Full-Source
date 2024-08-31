# PaymentSDK и Trust

PaymentSDK вызывает некоторые методы в Траст напрямую, некоторые - через свой бэкенд, который расположен в [mail/payments-sdk-backend](https://a.yandex-team.ru/arc_vcs/mail/payments-sdk-backend).
Логи бэкенда лежат тут: 
[Тестинг](https://deploy.yandex-team.ru/stages/payments-sdk-backend-testing/logs)
[Прод](https://deploy.yandex-team.ru/stages/payments-sdk-backend-production/logs)

Логи клиентской части PaymentSDK пишутся в аппметрику, если хост не отключил это. Лежат в `Hahn` в большой общей таблице `logs/appmetrica-yandex-events/`, фильтровать по APIKey для Android `3308482` и iOS `3429631`.
Небольшая вводная, как смотреть логи в аппметрике [тут](https://wiki.yandex-team.ru/users/a-kononova/paymentsdk/kak-bystro-nachat-smotret-logi-paymentsdk/)

## Хосты
Клиентская часть PaymentSDK ходит только в дайхард

Тестинг `https://pci-tf.fin.yandex.net/api/`
Прод `https://diehard.yandex.net/api/`

Бэкенд PaymentSDK ходит в Траст вот сюда

Тестинг:
* `https://trust-payments-test.paysys.yandex.net:8028/trust-payments/`
* `https://trust-paysys-test.paysys.yandex.net:8025/bindings-external/v2.0`

Прод:
* `https://trust-payments.paysys.yandex.net:8028/trust-payments/`
* `https://trust-paysys.paysys.yandex.net:8025/bindings-external/v2.0`

## Оплата
### Инициализация
При старте процесса оплаты клиентская часть идет в бэкенд PaymentSDK:
* Тестинг `https://mobpayment-test.yandex.net/v1/init_payment`
* Прод `https://mobpayment.yandex.net/v1/init_payment`
Бэкенд в свою очередь:
1. Если платеж через сервис Я.Оплаты, то сначала идем в бэкенд Оплат и обмениваем payment_token на трастовый purchase_token
1. Идёт в антифрод `https://trust-payments.paysys.yandex.net:8028/trust-payments/v2/payments/purchase_token/afs_payment_methods`
1. Идёт в Траст за списком методов `https://trust-payments.paysys.yandex.net:8028/trust-payments/v2/payment-methods`
1. Получает статус корзины `https://trust-payments.paysys.yandex.net:8028/trust-payments/v2/payments/purchase_token`
1. Стартует корзину 
    * Обычные платежи `https://trust-payments.paysys.yandex.net:8028/trust-payments/v2/payments/purchase_token/start`
    * Кредиты `https://trust-payments.paysys.yandex.net:8028/trust-payments/v2/credit/purchase_token/start`
1. По списку из антифрода определяется необходимость ввода cvv, все данные мержатся и отправляются в клиентскую часть PaymentSDK

Если что-то пошло не так, наш бэкенд вернёт ошибку на клиент, а также приложит туда request_id.

Можно посмореть по трейсам в Jaeger вот [тут](https://jaeger.yt.yandex-team.ru/trust/search?limit=20&lookback=1h&maxDuration&minDuration&operation=Payment%20initialization&service=payment-sdk-backend)

Как это выглядит в логах PaymentSDK:

Запрос к бэкенду PaymentSDK:
EventName | EventValue
:--- | :---
EVENTUS_initiated_payment | `{\"uid\":\"123456789\",\"eventus_id\":1630756637256,\"payment_token\":\"deadbeefdeadbeef\",\"sdk_version\":\"2.10.0\",\"event_name\":\"initiated_payment\",\"is_debug\":false,\"service_token\":\"blue_market_payments_xxx\",\"version\":1,\"payment_src\":\"ru.beru.android\",\"timestamp\":1630756637249}`

Запрос успешно завершился:
EventName | EventValue
:--- | :---
EVENTUS_initiated_payment_success | `{\"eventus_id\":1630756639055,\"payment_token\":\"deadbeefdeadbeef\",\"initialization_id\":\"1630756639044\",\"purchase_currency\":\"RUB\",\"is_debug\":false,\"timespan\":1795,\"version\":1,\"uid\":\"123456789\",\"purchase_token\":\"deadbeefdeadbeef\",\"origin_eventus_id\":1630756637256,\"sdk_version\":\"2.10.0\",\"event_name\":\"initiated_payment_success\",\"purchase_total_amount\":\"1024.00\",\"service_token\":\"blue_market_payments_xxx\",\"payment_src\":\"ru.beru.android\",\"timestamp\":1630756639044}`

### Выбор метода оплаты

**Вариант 1:** Обычная оплата через `supply_payment_data`

После успешной инициализации платежа пользователь видит список методов оплаты. Выбирает метод, нажимает "Оплатить" и PaymentSDK досылает платёжный метод в Траст через `https://diehard.yandex.net/api/supply_payment_data`. После этого PaymentSDK начинает поллинг статуса платежа.

Как это выглядит в логах PaymentSDK:

Все события имеют вид `EVENTUS_xxx_payment`, где xxx - название метода, например `new_card`, `existing_card`, `google_pay`, `apple_pay`.
EventName | EventValue
:--- | :---
EVENTUS_new_card_payment | `{\"eventus_id\":1630759342480,\"payment_token\":\"deadbeefdeadbeef\",\"initialization_id\":\"1630756639044\",\"purchase_currency\":\"RUB\",\"is_debug\":false,\"version\":1,\"uid\":\"123456789\",\"purchase_token\":\"deadbeefdeadbeef\",\"sdk_version\":\"2.10.0\",\"event_name\":\"new_card_payment\",\"purchase_total_amount\":\"1024.00\",\"bind_card\":true,\"service_token\":\"blue_market_payments_xxx\",\"payment_src\":\"ru.beru.android\",\"timestamp\":1630759342454}`

**Вариант 2:** Оплата при наличии Order Tag через GooglePay/ApplePay.

Этот вариант происходит если хостовое приложение, интегрирующее PaymentSDK, при оплате задало order tag и пользователь выбрал ApplePay/GooglePay.
1. Будет вызвана привязка Google `https://diehard.yandex.net/api/bind_google_pay_token` или Apple `https://diehard.yandex.net/api/bind_apple_token` токена соответственно.
1. PaymentSDK досылает платёжный метод в Траст через `https://diehard.yandex.net/api/supply_payment_data`.
1. Начинается поллинг статуса платежа.

Как в логах выглядит привязка токена:
EventName | EventValue
:--- | :---
EVENTUS_bind_google_pay | `{\"uid\":\"123456789\",\"eventus_id\":1632138452222,\"sdk_version\":\"2.10.0\",\"event_name\":\"bind_google_pay\",\"is_debug\":false,\"service_token\":\"taxifee_xxx\",\"version\":1,\"payment_src\":\"ru.yandex.yandexmaps\",\"timestamp\":1632138452583}`
EVENTUS_bind_google_pay_success | `{\"uid\":\"123456789\",\"origin_eventus_id\":1632138452222,\"eventus_id\":1632138452333,\"sdk_version\":\"2.10.0\",\"event_name\":\"bind_google_pay_success\",\"is_debug\":false,\"timespan\":1235,\"service_token\":\"taxifee_xxx\",\"version\":1,\"payment_src\":\"ru.yandex.yandexmaps\",\"timestamp\":1632138453735}`
Для ApplePay аналогично, только события называются `EVENTUS_bind_apple_pay` и `EVENTUS_bind_apple_pay_success`.

Рядом в логах будет поход в сервисы Google за токеном
EventName | EventValue
:--- | :---
EVENTUS_open_google_pay_dialog | `{\"uid\":\"123456789\",\"eventus_id\":1632138093424,\"sdk_version\":\"2.10.0\",\"event_name\":\"open_google_pay_dialog\",\"is_debug\":false,\"service_token\":\"taxifee_xxx\",\"version\":1,\"payment_src\":\"ru.yandex.yandexmaps\",\"timestamp\":1632138093378}`
EVENTUS_google_pay_token_received | `{\"uid\":\"123456789\",\"eventus_id\":1632138162211,\"sdk_version\":\"2.10.0\",\"event_name\":\"google_pay_token_received\",\"is_debug\":false,\"service_token\":\"taxifee_xxx\",\"version\":1,\"payment_src\":\"ru.yandex.yandexmaps\",\"timestamp\":1632138166129}`
Для ApplePay аналогично, только события называются `EVENTUS_apple_pay_authorization_view_controller_call` и `EVENTUS_apple_pay_token_received`.

**Вариант 3:** Кредиты Тинькофф.

Если платеж является кредитом от Тинькофф, тогда мы сразу показываем клиенту страничку банка. Если банк одобрил кредит, то переходим к этапу поллинга.

### Поллинг
После посылки платежного метода или аппрува кредита Тиньковым PaymentSDK начинает поллинг, вызывая `https://diehard.yandex.net/api/check_payment`:
1. При получении статуса `wait_for_notification` в ответе есть `redirect_3ds_url`, то показываем 3ds страничку, если ещё не показывали
1. При получении `success` или `wait_for_processing` (для кредитов Тинькофф) успешно завершаем поллинг.
1. Иначе завершаем поллинг с ошибкой

Как это выглядит в логах PaymentSDK:

Когда поллинг завершится, будет событие вида `EVENTUS_xxx_payment_success` или `EVENTUS_xxx_payment_failure` в зависимости от результата, плюс рядом будет событие закрытия диалога (если используется диалог) `EVENTUS_closed`.

EventName | EventValue
:--- | :---
EVENTUS_new_card_payment_failure | `{\"reason\":\"Diehard Error: status - payment_timeout, code - N/A, status_3ds - N/A, description - timeout while waiting for payment data\",\"eventus_id\":1630759343116,\"payment_token\":\"deadbeefdeadbeef\",\"initialization_id\":\"1630756639044\",\"purchase_currency\":\"RUB\",\"is_debug\":false,\"timespan\":631,\"error\":true,\"version\":1,\"uid\":\"123456789\",\"purchase_token\":\"deadbeefdeadbeef\",\"origin_eventus_id\":1630759342480,\"sdk_version\":\"2.10.0\",\"event_name\":\"new_card_payment_failure\",\"purchase_total_amount\":\"1024.00\",\"bind_card\":true,\"service_token\":\"blue_market_payments_xxx\",\"payment_src\":\"ru.beru.android\",\"timestamp\":1630759343085}`
EVENTUS_closed | `{\"reason\":\"Diehard Error: status - payment_timeout, code - N/A, status_3ds - N/A, description - timeout while waiting for payment data\",\"eventus_id\":1630759343114,\"payment_token\":\"deadbeefdeadbeef\",\"initialization_id\":\"1630756639044\",\"purchase_currency\":\"RUB\",\"is_debug\":false,\"version\":1,\"uid\":\"123456789\",\"purchase_token\":\"deadbeefdeadbeef\",\"sdk_version\":\"2.10.0\",\"event_name\":\"failed_payment\",\"purchase_total_amount\":\"1024.00\",\"service_token\":\"blue_market_payments_xxx\",\"payment_src\":\"ru.beru.android\",\"timestamp\":1630759343084}`

## Привязка карт

Поддержано два типа привязки, какой использовать решает само приложение при конфигурации PaymentSDK.

### Старый тип привязки
Посылается запрос в дайхард на метод `https://diehard.yandex.net/api/bind_card`.
Как это выглядит в логах PaymentSDK:

EventName | EventValue
:--- | :---
EVENTUS_bind_new_card | `{\"service_token\":\"\",\"payment_src\":\"ru.yandex.mobile.drive\",\"bind_version\":\"v1\",\"sdk_version\":\"2.10.0\",\"version\":1,\"event_name\":\"bind_new_card\",\"timestamp\":1631539069164,\"value\":\"411111******1111\",\"uid\":\"123456789\",\"is_debug\":false,\"eventus_id\":1631539069198}`
EVENTUS_bind_new_card_success | `{\"service_token\":\"\",\"uid\":\"123456789\",\"eventus_id\":1631539074175,\"origin_eventus_id\":1631539069198,\"event_name\":\"bind_new_card_success\",\"value\":\"411111******1111\",\"version\":1,\"payment_src\":\"ru.yandex.mobile.drive\",\"is_debug\":false,\"bind_version\":\"v1\",\"timespan\":4975,\"timestamp\":1631539074139,\"sdk_version\":\"2.10.0\"}`

### Новый тип привязки
1. Клиентская часть PaymentSDK шифрует данные карты и отправляет их в дайхард `https://diehard.yandex.net/api/bindings/v2.0/bindings`
1. Получив идентификатор привязки, отправляет его в бэкенд PaymentSDK
1. Бэкенд идет в Траст в метод `https://trust-paysys.paysys.yandex.net:8025/bindings-external/v2.0/bindings/идентификатор_привязки/verify/` получает `purchase_token` и передает его на клиент
1. Клиентская часть PaymentSDK начинает поллинг аналогично сценарию оплаты.

В логах PaymentSDK отличается значением параметра `bind_version`, плюс, если хост всё делает правильно, будет присутствовать сервис токен, в отличие от первого типа привязок где он необязателен.

Этап привязки:

EventName | EventValue
:--- | :---
EVENTUS_bind_new_card | `{\"bind_version\":\"v2\",\"timestamp\":1631515790932,\"service_token\":\"taxifee_xxx\",\"uid\":\"123456789\",\"version\":1,\"eventus_id\":1631515790955,\"value\":\"411111******1111\",\"sdk_version\":\"2.10.0\",\"is_debug\":false,\"event_name\":\"bind_new_card\",\"payment_src\":\"com.appkode.foodfox\"}`
EVENTUS_bind_new_card_binding | `{\"timestamp\":1631515790941,\"version\":1,\"event_name\":\"bind_new_card_binding\",\"eventus_id\":1631515790965,\"uid\":\"123456789\",\"service_token\":\"taxifee_xxx\",\"sdk_version\":\"2.10.0\",\"payment_src\":\"com.appkode.foodfox\",\"is_debug\":false}`
EVENTUS_bind_new_card_binding_success | `{\"payment_src\":\"com.appkode.foodfox\",\"eventus_id\":1631515793125,\"origin_eventus_id\":1631515790965,\"is_debug\":false,\"event_name\":\"bind_new_card_binding_success\",\"version\":1,\"timespan\":2159,\"sdk_version\":\"2.10.0\",\"service_token\":\"taxifee_xxx\",\"uid\":\"123456789\",\"timestamp\":1631515793100}`

Этап поллинга:

EventName | EventValue
:--- | :---
EVENTUS_bind_new_card_success | `{\"service_token\":\"\",\"uid\":\"123456789\",\"eventus_id\":1631539074175,\"origin_eventus_id\":1631539069198,\"event_name\":\"bind_new_card_success\",\"value\":\"411111******1111\",\"version\":1,\"payment_src\":\"ru.yandex.mobile.drive\",\"is_debug\":false,\"bind_version\":\"v1\",\"timespan\":4975,\"timestamp\":1631539074139,\"sdk_version\":\"2.10.0\"}`

### Отвязка карт
Посылается запрос в дайхард на метод `https://diehard.yandex.net/api/unbind_card`.

Как это выглядит в логах PaymentSDK:

EventName | EventValue
:--- | :---
EVENTUS_unbind_card | `{\"eventus_id\":1631481559220,\"service_token\":\"payment_sdk_xxx\",\"event_name\":\"unbind_card\",\"timestamp\":1631481559205,\"is_debug\":true,\"version\":1,\"card_id\":\"card-xdeadbeefdeadbeef\",\"uid\":\"123456789\",\"sdk_version\":\"2.11.0\",\"payment_src\":\"com.yandex.mobilepaymentsdk.sample\"}`
EVENTUS_unbind_card_success | `{\"card_id\":\"card-xdeadbeefdeadbeef\",\"sdk_version\":\"2.11.0\",\"timespan\":312,\"version\":1,\"origin_eventus_id\":1631481559220,\"uid\":\"123456789\",\"eventus_id\":1631481559533,\"payment_src\":\"com.yandex.mobilepaymentsdk.sample\",\"event_name\":\"unbind_card_success\",\"service_token\":\"payment_xxx\",\"timestamp\":1631481559517,\"is_debug\":true}`