## Куб смотрений

Одна строчка в кубе = одна единица просмотренного пользователем контента.

Описание построения: https://st.yandex-team.ru/SDA-61

Путь до таблиц на YT: `//home/sda/cubes/tv/watching/{date}`

## Описание полей таблицы

+ `app_version` - версия прошивки
+ `board` - производитель платы из поля `app_version`
+ `build` - номер билда из поля `app_version`
+ `channel_id` - номер канала
+ `channel_name` - название канала
+ `channel_type` - тип канала
+ `clid1` - клид активации телевизора
+ `clid100010` - клид видеосмотрения
+ `content_id` - content_id просмотренного контента
+ `content_type` - тип контента (tv / native)
+ `device_id` - метричный device_id пользователя
+ `diagonal` - диагональ устройства
+ `fielddate` - дата
+ `firmware_version` - человекочитаемая версия прошивки из поля `app_version`
+ `manufacturer` - производитель
+ `model` - модель устройства
+ `monetization_model` - нормализованная модель монетизации контента (avod, svod, tvod_est), вычислияется по полю license
+ `parent_id` - id карусели, с которой произошел просмотр
+ `platform` - базовая платформа из поля `app_version`
+ `player_session_id` - video-session-id
+ `puid` - puid залогиненного пользователя
+ `resolution` - разрешение устройства
+ `session_id` - кастомная сессия tvandroid (от момента включения до выключения устройства)
+ `url` - url просмотренного контента
+ `view_type` - тип смотрения (live / dvr / vod) (только для данных их STRM)
+ `view_time` - время смотрения в секундах
+ `view_source` - тип просмотренного контента
+ `test_buckets` - бакеты для экспериментов
