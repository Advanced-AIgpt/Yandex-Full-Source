# Сбор корзин для тренировки и валидации CatBoost модели Дискавери
## Пререквизиты
> `python==2.7, pip>=8.0.0` желательно в чистом *virtual env* (или *conda env*)

Установить зависимости можно с помошью
```
pip install -r requirements.txt
```

## Клики на карточку дискавери
### Пример запуска
``` bash
python clicks.py --date '2019-11-11'
# результат будет лежать на YT "hahn.//home/paskills/discovery/datasets/clicks/2019-11-11"
```
Так же можно запустить расчет сразу на несколько табличек одновременно
``` bash
python clicks.py --start-date '2019-10-01' --end-date '2019-10-31'
```


## Длинные сессии навыка
### Пример запуска
``` bash
python long_sessions.py --date '2019-11-11'
# результат будет лежать на YT "hahn.//home/paskills/discovery/datasets/long-sessions/2019-11-11"
```
Так же можно запустить расчет сразу на несколько табличек одновременно
``` bash
python long_sessions.py --start-date '2019-10-01' --end-date '2019-10-31'
```