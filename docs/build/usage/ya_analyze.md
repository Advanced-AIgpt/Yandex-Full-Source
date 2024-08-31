# ya analyze-make timeline : что и сколько времени заняло при сборке


Посмотреть профиль сборки.

`ya analyze-make timeline [options]`

Каждый запуск ya make пишет eventlog в `~/.ya/evlogs/`. Команда `ya analyze-make timeline` команда строит трейс, в одном из двух форматов. По умолчанию он генерирует трейсы, которые умеет показывать Яндекс Браузер (через `browser://tracing`) и Chrome (через `chrome://tracing/`). Альтернативный формат совместим с maptplotlib.

По умолчанию берётся последний трейс из `~/.ya/evlogs/` на текущую дату, но можно подать файл явно.

Также можно посмотреть профиль сборки DistBuild, подав на вход с опцией ```--distbuild-json-from-yt``` JSON, полученный с помощью ```devtools/distbuild/sre/simple_build_simulator/yql``` 

## Пример
```
$ ./ya analyze-make timeline
Open about://tracing in Chromium and load 14-27-47.ndpzv5xlrled702w.evlog.json file.
```
![Трейс](https://jing.yandex-team.ru/files/spreis/analyze-make-trace.PNG "Трейс в Яндекс Браузер" =400x200)


## Опции
```
--distbuild-json-from-yt=ANALYZE_DISTBUILD_FILE   Анализировать таблицу YT в виде json 
--evlog=ANALYZE_EVLOG_FILE                        Анализировать лог из файла
--format=OUTPUT_FORMAT                            Формат вывода, по умолчанию chromium_trace 
--plot                                            Формат вывода plot (matplotlib)
-h, --help                                        Справка
```
