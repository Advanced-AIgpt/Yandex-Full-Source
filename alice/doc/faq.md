# Частые вопросы о разработке сценариев

## Поддержка {#support}
### Есть ли у вас чат поддержки в телеграме?
Да есть, и даже несколько:
* [Чат](https://t.me/joinchat/BBPaeRGYwx3cNiNYXnK0SQ) поддержки [Megamind](megamind/index.md) и для общих вопросов.
* [Чат](https://t.me/+DJryz0GP71djN2Ri) поддержки [NLU](nlu/index.md).
* [Чат](https://t.me/joinchat/WLI9XSTsGdIdeek2) поддержки [Hollywood](hollywood/index.md).
* [Чат](https://t.me/joinchat/BwkfgEvuehoNBLmukS-CaA) поддержки [тестирования](testing/index.md) Алисы (релизы и мержи в ветки).

### Есть ли рассылка где я могу узнавать новости?
Да, есть канал в телеграме с новостями для разработчиков: [https://t.me/joinchat/AAAAAFUSQnMICl_NtYe6jg](https://t.me/joinchat/AAAAAFUSQnMICl_NtYe6jg)

## Разработка
### Есть ли общепринятый механизм, как узнать, есть ли у пользователя подписка на Яндекс.Плюс?
Да, можно подписаться в [конфиге](megamind/config#format) на [датасорс](architecture#sources) `BLACK_BOX`, среди полей есть данные наличии подписки [HasYandexPlus](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/blackbox/blackbox.proto?rev=7141810#L16)

### Как подменить урл чего-нибудь где-нибудь
Подробно описано [здесь](testing/srcrwr.md)

### Как активировать эксперимент из аб
Можно переопределить урл uniproxy добавив в него test-id `?test-id=123&test-id=456`, например:
`wss://beta.uniproxy.alice.yandex.net/alice-uniproxy-hamster/uni.ws?test-id=123`
