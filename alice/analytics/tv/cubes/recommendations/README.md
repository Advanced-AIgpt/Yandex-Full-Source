## Куб рекомендаций

Одна строчка в кубе = одна единица совершенного пользователем действия.
В кубе три типа действий -- показ карточки, клик по карточке, скролл карусели.

Путь до таблиц на YT: `//home/sda/cubes/tv/recommendations/logs/{date}`

## Описание полей таблицы

+ `app_version` - версия прошивки
+ `board` - производитель платы из поля `app_version`
+ `build` - номер билда из поля `app_version`
+ `carousel_position` - позиция карусели
+ `clid1` - клид активации телевизора
+ `clid100010` - клид видеосмотрения
+ `content_card_position` - позиция карточки в карусели
+ `content_id` - id карточки
+ `content_type` - тип карточки
+ `device_id` - метричный device_id пользователя
+ `diagonal` - диагональ устройства
+ `event_name` - название действия
+ `event_timestamp` - timestamp события
+ `fielddate` - дата
+ `firmware_version` - человекочитаемая версия прошивки из поля `app_version`
+ `has_plus` - наличие подписки Плюс
+ `is_logged_in` - флаг залогиненности
+ `manufacturer` - производитель
+ `model` - модель устройства
+ `parent_id` - id карусели
+ `place` - имя экрана
+ `platform` - базовая платформа из поля `app_version`
+ `player_session_id` - video-session-id
+ `resolution` - разрешение устройства
+ `session_id` - кастомная сессия tvandroid (от момента включения до выключения устройства)
+ `test_buckets` - бакеты для экспериментов

## Доступная информация о контенте

+ `actors`
+ `countries`
+ `director`
+ `duration`
+ `genres`
+ `kinopoisk_id`
+ `kp_rating`
+ `onto_id`
+ `release_date`
+ `title`
