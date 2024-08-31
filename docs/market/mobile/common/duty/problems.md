# Основные сценарии проблем

{% note warning %}

Ты должен быть в курсе любой проблемы, даже если это не в мобилках

{% endnote %}

## Превышен процент ошибок запросов (данные от nginx)
1. Смотрим графики в поисках компонента, из-за которого ошибки

   1.1 Если ошибки в КАПИ:

   [График ошибок](https://grafana.yandex-team.ru/d/000016910/blue-market-apps-pulse?panelId=9&orgId=1&refresh=1m&fullscreen)

   [График ошибок по ручкам](https://grafana.yandex-team.ru/d/000016910/blue-market-apps-pulse?orgId=1&refresh=1m&fullscreen&panelId=17)

   [График ошибок по компонентам](https://grafana.yandex-team.ru/d/FmGGlPOiz/market-content-api?refresh=1m&orgId=1&fullscreen&panelId=13)

   [Графики ошибок по компонентам/дата-центрам (Вкладка Main)](https://grafana.yandex-team.ru/d/FmGGlPOiz/market-content-api?refresh=1m&orgId=1)

   Если не понятно, в каком компоненте проблема, то пиши в [чат](https://t.me/joinchat/C50Gxz7Gow-aN4NlwQc0rQ) КАПИ, ребята помогут разобраться, а ты спроси у них, как они локализовали проблему, после того, как всё решится, потому что нужно учиться.

   1.2 Если ошибки в ФАПИ:

   [График ошибок](https://grafana.yandex-team.ru/d/000016910/blue-market-apps-pulse?orgId=1&refresh=1m&fullscreen&panelId=51)

   [График ошибок по резолверам (5XX)](https://grafana.yandex-team.ru/d/000016910/blue-market-apps-pulse?panelId=63&orgId=1&refresh=1m&fullscreen)

   [График ошибок по резолверам (4XX)](https://grafana.yandex-team.ru/d/000016910/blue-market-apps-pulse?orgId=1&refresh=1m&fullscreen&panelId=64)

   [Все графики ФАПИ](https://grafana.yandex-team.ru/d/h6WyufZZz/blue-market-fapi-pulse?orgId=1&refresh=1m)

   [Чат](https://t.me/joinchat/BNcTYEBNnV9wg3PI-55eIQ) фронтов

2. Узнав, в каком компоненте ошибка, идём в чат компонента.
Все чаты [здесь](https://wiki.yandex-team.ru/Market/mobile/marketapps/Dezhurstva/#chatikiigrafikikomponentovmarketa)

3. Далее по чеклисту из тикета

## Превышен процент ошибочных запросов у пользователей (данные из приложений)

1. Смотрим графики в поисках компонента, из-за которого ошибки

   1.1 Если ошибки в КАПИ:

   [Пример запроса](https://yql.yandex-team.ru/Operations/Xr5sVJdg8sROSW9ndS7CWq4v-eWCR87Np2s6XTK4BSs=)

   [График ошибок](https://grafana.yandex-team.ru/d/000016910/blue-market-apps-pulse?orgId=1&refresh=1m&fullscreen&panelId=87)

   [График ошибок по ручкам](https://grafana.yandex-team.ru/d/000016910/blue-market-apps-pulse?orgId=1&refresh=1m&fullscreen&panelId=17)

   [График ошибок по компонентам](https://grafana.yandex-team.ru/d/FmGGlPOiz/market-content-api?refresh=1m&orgId=1&fullscreen&panelId=13)

   [Графики ошибок по компонентам/дата-центрам (Вкладка Main)](https://grafana.yandex-team.ru/d/FmGGlPOiz/market-content-api?refresh=1m&orgId=1)

   Если не понятно, в каком компоненте проблема, то пиши в [чат](https://t.me/joinchat/C50Gxz7Gow-aN4NlwQc0rQ) КАПИ, ребята помогут разобраться, а ты спроси у них, как они локализовали проблему, после того, как всё решится, потому что нужно учиться.

   1.2 Если ошибки в ФАПИ:

   [Пример запроса](https://yql.yandex-team.ru/Operations/XuYqmGim9YmO_BHw1_2UvHz18_4VuKAB9aajpfLAm7g=)

   [Группировка по резолверу](https://yql.yandex-team.ru/Operations/XuYq_VPzVKe0VMt5GqvvGWnEQ77dkHHK1lae9lgVUfo=)

   [График ошибок](https://grafana.yandex-team.ru/d/000016910/blue-market-apps-pulse?orgId=1&refresh=1m&fullscreen&panelId=48)

   [График ошибок по резолверам (5XX)](https://grafana.yandex-team.ru/d/000016910/blue-market-apps-pulse?panelId=63&orgId=1&refresh=1m&fullscreen)

   [График ошибок по резолверам (4XX)](https://grafana.yandex-team.ru/d/000016910/blue-market-apps-pulse?orgId=1&refresh=1m&fullscreen&panelId=64)

   [Все графики ФАПИ](https://grafana.yandex-team.ru/d/h6WyufZZz/blue-market-fapi-pulse?orgId=1&refresh=1m)

   [Чат](https://t.me/joinchat/BNcTYEBNnV9wg3PI-55eIQ) фронтов

2. Узнав, в каком компоненте ошибка, идём в чат компонента.
Все чаты [здесь](https://wiki.yandex-team.ru/Market/mobile/marketapps/Dezhurstva/#chatikiigrafikikomponentovmarketa)

3. Далее по чеклисту из тикета

## Повысились тайминги

1. Смотрим графики в поисках компонента, из-за которого повысились тайминги

   1.1 Если тайминги в КАПИ:

   [График таймингов](https://grafana.yandex-team.ru/d/000016910/blue-market-apps-pulse?orgId=1&refresh=1m&fullscreen&panelId=5)

   [График таймингов по ручкам](https://grafana.yandex-team.ru/d/000016910/blue-market-apps-pulse?orgId=1&refresh=1m&fullscreen&panelId=107)

   [График таймингов по компонентам](https://grafana.yandex-team.ru/d/FmGGlPOiz/market-content-api?refresh=1m&orgId=1&fullscreen&panelId=185)

   [Графики ошибок по компонентам/дата-центрам (Вкладка External services timings)](https://grafana.yandex-team.ru/d/FmGGlPOiz/market-content-api?refresh=1m&orgId=1)

   Если не понятно, в каком компоненте проблема, то пиши в [чат](https://t.me/joinchat/C50Gxz7Gow-aN4NlwQc0rQ) КАПИ, ребята помогут разобраться, а ты спроси у них, как они локализовали проблему, после того, как всё решится, потому что нужно учиться.

   1.2 Если тайминги в ФАПИ:

   [График таймингов](https://grafana.yandex-team.ru/d/000016910/blue-market-apps-pulse?orgId=1&refresh=1m&fullscreen&panelId=50)

   [График таймингов по резолверам](https://grafana.yandex-team.ru/d/000016910/blue-market-apps-pulse?orgId=1&refresh=1m&fullscreen&panelId=71)

   [Все графики ФАПИ](https://grafana.yandex-team.ru/d/h6WyufZZz/blue-market-fapi-pulse?orgId=1&refresh=1m)

   [Чат](https://t.me/joinchat/BNcTYEBNnV9wg3PI-55eIQ) фронтов

2. Узнав, в каком компоненте ошибка, идём в чат компонента.
Все чаты [здесь](https://wiki.yandex-team.ru/Market/mobile/marketapps/Dezhurstva/#chatikiigrafikikomponentovmarketa)

3. Далее по чеклисту из тикета

## Что-то идёт не так

[График iOS](https://grafana.yandex-team.ru/d/000016910/blue-market-apps-pulse?orgId=1&refresh=1m&fullscreen&edit&panelId=89)

[График Android](https://grafana.yandex-team.ru/d/000016910/blue-market-apps-pulse?orgId=1&refresh=1m&fullscreen&panelId=90)

[Пример запроса](https://yql.yandex-team.ru/Operations/Xr5zWGim9YAMS0CSjZS85f62EweuPjMMPTNymPmlj1s=)

Обычно SGW идут рука об руку с другими ошибками.
По ним и можно вычислить быстрее, что за ошибка.

[Пример](https://yql.yandex-team.ru/Operations/XrAdVmHljlEOqdk0LpUzvKb4Jz3NOtArFW-A6nrAS_Y=) расследования `SOMETHING_GOES_WRONG` со связанными событиями

[График всех ошибок iOS](https://grafana.yandex-team.ru/d/000016910/blue-market-apps-pulse?orgId=1&refresh=1m&fullscreen&panelId=43)

[График всех ворнингов iOS](https://grafana.yandex-team.ru/d/000016910/blue-market-apps-pulse?orgId=1&refresh=1m&fullscreen&panelId=77)

[График всех ошибок Android](https://grafana.yandex-team.ru/d/000016910/blue-market-apps-pulse?orgId=1&refresh=1m&fullscreen&panelId=44)

[График всех ворнингов Android](https://grafana.yandex-team.ru/d/000016910/blue-market-apps-pulse?orgId=1&refresh=1m&fullscreen&panelId=73)

## Пустые серпы

[График iOS](https://grafana.yandex-team.ru/d/000016910/blue-market-apps-pulse?orgId=1&refresh=1m&fullscreen&edit&panelId=89)

[График Android](https://grafana.yandex-team.ru/d/000016910/blue-market-apps-pulse?orgId=1&refresh=1m&fullscreen&panelId=90)

[Пример запроса](https://yql.yandex-team.ru/Operations/Xr5vzZ3udnbbPLsppJRZIjFzAQnUDPVuWA202gbCdkg=)

Если выдача не должна быть пустой на такой запрос, то ищем по uuid запросы пользователя (см страницу с запросами), и смотрим трассировку запросов.
Запросы в репорт всегда можно воспроизвести (через `curl` и `ssh` `public`). Если выдача от репорта и сейчас пустая, то значит репорт барахлит.

Если выдача на данные запросы должна быть, то пишем в [репорт](https://t.me/joinchat/BPRBpktgmbEKCVsQxWfF_g)

## Ошибки таймаута

[График ФАПИ](https://grafana.yandex-team.ru/d/000016910/blue-market-apps-pulse?orgId=1&refresh=1m&fullscreen&panelId=37)

[Пример запроса на ФАПИ](https://yql.yandex-team.ru/Operations/Xr52IJ3udnbbPL9gMHcVGDWbDcFVUbXmS4Sr47AdoTk=)

[Все графики ФАПИ](https://grafana.yandex-team.ru/d/h6WyufZZz/blue-market-fapi-pulse?orgId=1&refresh=1m)

[Чат](https://t.me/joinchat/BNcTYEBNnV9wg3PI-55eIQ) фронтов

Данная ошибка говорит о том, что ФАПИ недоступен для пользователей.
Можно сходить в [чат](https://telegram.me/joinchat/ByTNYD79mq1FOGJkLYsq5A) админов, спросить, всё ли хорошо с сетью.

## Ошибки оплаты кредитом

[График](https://grafana.yandex-team.ru/d/000016910/blue-market-apps-pulse?orgId=1&refresh=1m&fullscreen&panelId=46)

[Пример запроса](https://yql.yandex-team.ru/Operations/YCQ2LfMBw4rPSVGWr790SyYRqVGU4fb5nFFToN_DAIA=)

Проблемы могут быть у чекаутера. Их [чат](https://t.me/joinchat/AAAAAEBjpLkyEvSEoup4_Q).

Если не у них, то можно спросить в чате [MarketMobile](https://t.me/joinchat/ChXJS0p1hZsodN6Lor9NWA), кто может помочь со Сбером.

## Ошибки оплаты картой

[График](https://grafana.yandex-team.ru/d/000016910/blue-market-apps-pulse?orgId=1&refresh=1m&fullscreen&panelId=46)

[Пример запроса](https://yql.yandex-team.ru/Operations/YDZNyNK3DINFIyuPrzs7UGnW-_tgo8rQZP50mdJojAU=)

Если коды ошибок дали понять, у кого проблемы, хорошо. Можно идти к ответственным.
- [Траст](https://t.me/joinchat/A3W8CRXtWc248d5kBsZJSA)
- [Баланс](https://t.me/joinchat/AAAAAD7dn-s9vLcHgXB4qg)
- [Чекаутер](https://t.me/joinchat/AAAAAEBjpLkyEvSEoup4_Q)
- [Payment SDK](https://t.me/joinchat/ECJJh1Cv6Jj2g-r7XpCgwg)

## Ошибки оплаты GooglePay

[График](https://grafana.yandex-team.ru/d/000016910/blue-market-apps-pulse?orgId=1&refresh=1m&fullscreen&panelId=46)

[Пример запроса](https://yql.yandex-team.ru/Operations/YDZN0tK3DINFIyufWcWOmuSt2ttptypUsucWrC3t-J8=)

Проблемы могут быть у чекаутера. Их [чат](https://t.me/joinchat/AAAAAEBjpLkyEvSEoup4_Q).

Если не у них, то можно спросить в чате , кто может помочь с GooglePay.

Также, если сыпятся ошибки от PSDK, то нужно идти в чат поддержки [Payment SDK](https://t.me/joinchat/ECJJh1Cv6Jj2g-r7XpCgwg)

## Ошибки оплаты ApplePay

[График](https://grafana.yandex-team.ru/d/000016910/blue-market-apps-pulse?orgId=1&refresh=1m&fullscreen&panelId=46)

[Пример запроса](https://yql.yandex-team.ru/Operations/Xr5vEpdg8sROSXFFB-6Uu3ebsJLp9VeqoRobxA43ZQQ=)

Проблемы могут быть у чекаутера. Их [чат](https://t.me/joinchat/AAAAAEBjpLkyEvSEoup4_Q).

Если не у них, то можно спросить в чате [MarketMobile](https://t.me/joinchat/ChXJS0p1hZsodN6Lor9NWA), кто может помочь с ApplePay.

## Критические ошибки

[График](https://grafana.yandex-team.ru/d/000016910/blue-market-apps-pulse?orgId=1&refresh=1m&fullscreen&panelId=19)

[Пример запроса](https://yql.yandex-team.ru/Operations/Xr53wZ3udnbbPMCnKUjAbKP79G7zqnub0IHpOromXv8=)

Обычно в этом графике креши.
Нужно посмотреть запрос, что за креши и дальше разбираться по аналогии с [пунктом](https://docs.yandex-team.ru/market-mobile/common/duty/problems#kreshi)

## Ошибки добавления в корзину

[График iOS](https://grafana.yandex-team.ru/d/000016910/blue-market-apps-pulse?orgId=1&refresh=1m&fullscreen&edit&panelId=89)

[График Android](https://grafana.yandex-team.ru/d/000016910/blue-market-apps-pulse?orgId=1&refresh=1m&fullscreen&panelId=90)

[Пример запроса](https://yql.yandex-team.ru/Operations/Xr54mxpqv335TFaSUe4cOLx5KNjCRP2yNK_EizpjrVU=)

Это очень плохая ошибка. Нужно копать как можно быстрее.

Если есть повышение ошибок в запросах, то пишем в [КАПИ](https://t.me/joinchat/C50Gxz7Gow-aN4NlwQc0rQ), и в [чекаутер](https://t.me/joinchat/AAAAAEBjpLkyEvSEoup4_Q)

## Ошибки Деградации Loyalty

Ошибка связана с доступностью сервиса лоялти (скидки / промо)

[Пример запроса](https://yql.yandex-team.ru/Operations/YOmZbfMBw-ULAvhtByqIW_HwDh1SaHRgg3VXV-8F-FE=)

от туда можно достать request_id и посмотреть трассировку для локализации проблемы

[Хотлайн чат лоялти](https://t.me/joinchat/BPRBpkrRh_nkZy-GxzogxA)

Там можно задать вопросы по этой ошибке, но до этого нужно посмотреть трассировки 

## Креши

[Ссылки на метрику и Firebase](https://wiki.yandex-team.ru/Market/mobile/marketapps/Dezhurstva/#ssylki1)

### Если креши по всем версиям
Такой момент. В раскатанных версиях креш не может просто взяться из ниоткуда.

Сам выясняй, в какой функциональности креш, и попроси, чтобы посмотрели, кто недавно катался в ЦУМе

Есть варианты:
* включили эксперимент. Недавние выкатки экспериментов в [ЦУМе](https://tsum.yandex-team.ru/timeline/?from=now%2Fd&to=now&types=exp)
* [обновили](https://tsum.yandex-team.ru/timeline/?projects=market_cms) CMS
* [релизнули](https://tsum.yandex-team.ru/timeline/?projects=Content%20API) КАПИ
* [релизнули](https://tsum.yandex-team.ru/timeline/?tags=nanny-resource%3AMARKET_FRONT_API&types=nanny-deploy) ФАПИ
* релизнулись другие компоненты. Нужно искать, нужно подумать, от кого зависит функционал, где креш. Если есть идеи, идём туда

### Креши в раскатываемой версии

1. Нужно понять, можем ли мы отключить функциональность, в которой креш.
Если можем, то отключаем. Например через Firebase.

2. Если не можем, то останавливаем раскатку и запускаем хотфикс.

{% note warning %}

в iOS ещё нужно отключить виджет soft-update, так как версия всё равно будет доступна пользователям

{% endnote %}

3. Пишем публикатору или релизному менеджеру
