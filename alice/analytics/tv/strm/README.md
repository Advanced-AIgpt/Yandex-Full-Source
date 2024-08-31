# Отфильтрованные и обогащенные логи STRM

Фильтруется исходный STRM куб по полю `ref_from == tvandroid`.
Затем дополнительно обогащается данными из пользовательских параметров из поля `add_info`

Одна строчка в логе = одна сессия смотрения (vsid)

Исходные данные: `//cubes/video-strm/{date}/sessions`
Путь до таблиц на YT: `//home/smarttv/logs/strm/1d/{date}`

## Описание полей таблицы

+ `device_id` - метричный device_id пользователя
+ `session_id` - кастомная сессия tvandroid (от момента включения до выключения устройства)
+ `fielddate` - дата просмотра
+ `user_id` - (as is) - идентификатор пользователя на основе yandexuid
+ `yandexuid` - (as is) идентификатор пользователя
+ `puid` - (as is) паспортный идентификатор пользователя
+ `vsid` - (as is) video-session-id
+ `reqid` - (as is) reqid запроса
+ `browser_version` - (as is)
+ `video_content_id` - (as is) идентификатор видео контента
+ `region` - (as is) - регион смотрения
+ `view_type` - (as is) - тип смотрения (live / dvr / vod)
+ `add_info` - (as is) - json с дополнительными параметрами о просмотре
+ `errors` - (as is) - список ошибок плеера
+ `test_buckets` - (as is) - список экспериментальных бакетов
+ `timestamp` - (as is) - timestamp запуска плеера
+ `channel_id` - (as is) - идентификатор канала
+ `channel` - (as is) - название канала
+ `os_family` - (as is) семейство OS
+ `view_time` - (as is) - время смотрения в секундах
+ `player_events` - (as is) - события плеера
+ `browser_name` - (as is) - браузер
+ `user_agent` - (as is) - строка UserAgent
+ `manufacturer` - производитель телевизора
+ `model` - модель телевизора
+ `app_version` - версия прошивки в формате `{board}@{platform}@{firmware_version}@{build}` для устройств, начиная с версии 1.2.1
+ `board` - производитель платы из поля `app_version`
+ `platform` - базовая платформа из поля `app_version`
+ `firmware_version` - человекочитаемая версия прошивки из поля `app_version`
+ `build` - номер билда из поля `app_version`
+ `diagonal` - диагональ экрана устройства
+ `resolution` - разрешение экрана устройства
+ `clid1` - клид активации телевизора
+ `clid100010` - клид видеосмотрения
+ `channel_type` - тип канала (ott / other_vh), определяется по названию канала
+ `content_type` - тип контента (tv / native) из add_info
+ `carousel_name` - название карусели, с которой произошёл запуск видео - из параметра startup_place
+ `carousel_position` - позиция карусели, с которой произошёл запуск на экране (y) - из параметра startup_place
+ `content_card_position` - позиция тумбы в карусели, с которой произошёл запуск - - из параметра startup_place
+ `tv_screen` - название экрана, с которого произошёл запуск - из параметра startup_place
+ `tv_parent_screen` - название родительского экрана, с которого произошёл запуск (всегда - из списка основных экранов ТВ) - из параметра startup_place
+ `parent_id` - id карусели
+ `monetization_model` - нормализованная модель монетизации контента (avod, svod, tvod_est), вычислияется по полю license

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
