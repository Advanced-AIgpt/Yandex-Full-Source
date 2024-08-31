Dummy прокачка.
Используйте её, если нужно прокачать небольшое количество запросов "как в станции" или "как в ПП".
Пожалуйста не используйте данный скрипт в продакшен процессах, он не предназначен для этого. Если у вас большая корзина, пользуйтесь, пожалуйста, кубиками прокачки в Нирване(```VINS downloader```, ```Uniproxy downloader```). Для больших прокачек они более стабильны.

```
ya make alice/acceptance/modules/request_generator/scrapper/bin && ya make search/scraper_over_yt/mapper
cd $(ya dump root)/alice/acceptance/cli/dummy_downloader
ya make
./dummy_downloader --help
```
и запустите с нужными параметрами.

Формат входной корзины и более подробная документация по прокачкам: https://wiki.yandex-team.ru/Alice/Downloaders/
