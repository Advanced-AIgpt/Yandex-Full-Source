### Скриншотилка

**Документация:** https://wiki.yandex-team.ru/alice/downloaders/render-div-cards/

**Актуальный div2html рендерер:**
* репозиторий: https://github.yandex-team.ru/4eb0da/div2html/blob/master/dialog.html
* url в S3: http://div2html.s3.yandex.net/div12_2.html

**Структура проекта:**
* `bin/` - старая скриншотилка на Роторе (не поддерживается, оставлено для истории)
* `bin_vanadium/` - подготовка данных для Vanadium (работает в актуальной скриншотилке)
* `lib/` - общие библиотеки для скриншотилок
* `upload/` - cli для заливания рендереров (json->html) в S3
* `revision/` - ревизия стабильной версии кода, которая подтягивается в кубик скриншотилки
* `div2_card_dialog.json` - пример диалога с пользователя с Алисой с div2-карточкой
