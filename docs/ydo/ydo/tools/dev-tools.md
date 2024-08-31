# Инструменты разработки

### Стенд с кодом из мастера
[https://renderer-ydo-dev-master.hamster.yandex.ru/uslugi/](https://renderer-ydo-dev-master.hamster.yandex.ru/uslugi/)

### Storybook с компонентами
###### В каждом PR создаётся Storybook для текущих изменений

`https://frontend-test.s3.mds.yandex.net/story/@yandex-int/ydo/pull-XXX/index.html` (XXX-номер текущего PR)

###### Версия Storybook для master-ветки

[https://frontend-test.s3.mds.yandex.net/story/@yandex-int/ydo/master/index.html](https://frontend-test.s3.mds.yandex.net/story/@yandex-int/ydo/master/index.html)

### Создание exp flags
Все флаги используемые в приложении должны иметь описание в виде `.json` файла в `./expflags`. \
Для автоматического создания описания флагов существует `npx expflags` - интерактивная cli тулза.

### Анализ размера JS бандлов
1. `YENV=production npm run build:stats`
2. `npm run build:analyze`

В дополнение к этому, данный отчет доступен в каждом PR в проверке `PulseStatic`.

### Дебаг Redux
Для дебага redux store используется расширение `redux-devtools`. Необходимо установить [расширение для браузера](https://chrome.google.com/webstore/detail/redux-devtools/lmhkpmbekcpmknklioeibfkpmmfibljd?hl=ru).
Инструмент активен в development и testing сборках.
