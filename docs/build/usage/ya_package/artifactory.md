# Загрузка пакетов в artifactory
Загрузка пакетов в `artifactory` с помощью `ya package` может осуществляться как локально, так и в sandbox-таске `YA_PACKAGE(_2)`

Для загрузки пакета в `artifactory`, вам потребуется создать и заполнить
`settings.xml` - файл с описанием опций, нужных для заливки данных в `artifactory`.
[Пример](https://a.yandex-team.ru/arcadia/devtools/ya/package/tests/artifactory_upload/data/hello_settings_2.xml)

### Общая информация
* При загрузке файлов в `artifactory`, используется `deploy-file` плагин для `maven`
* `settings.xml` - это файл настроек, использующийся в `Maven Deploy Plugin`
* В `settings.xml` можно задекларировать нужные опции, которые, без использования `ya package` передавались бы в `maven`
* В `settings.xml` файле доступна подстановка версии пакета, которая берется из вашего `package.json`. [Пример](https://a.yandex-team.ru/arcadia/devtools/ya/package/tests/artifactory_upload/data/good_settings.xml) `settings.xml` c использованием подстановки.

### Локальная загрузка данных в artifactory
Для локальной загрузки данных в `artifactory`, достаточно к обычному запуску ya package передать опции `--artifactory` и `--publish-to`.
В опцию `--publish-to` нужно передать путь до `settings.xml` файла. Путь принимается как абсолютный, так и относительный от корня аркадии.
Для передачи пароля, следует использовать опцию `--artifactory-password-path`, куда следует передать путь до файла с паролем.
### Загрузка данных в artifactory из sandbox
* Поставьте галочку напротив поля `Upload package to artifactory`
* Укажите путь до `settings.xml` файла, относительно корня аркадии в секции `Publish to`
* В поле `Secret with artifactory password` выберите `yav`-секрет с паролем от `artifactory` и выберите нужный ключ.
* В качестве `dns`, во вкладке `advanced`, выберите `dns64`
