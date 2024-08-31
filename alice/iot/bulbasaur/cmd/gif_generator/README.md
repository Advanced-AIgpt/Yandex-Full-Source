# Генерация гифок для сценария голосового опроса

## Генерируем гифки из шаблонов
1. Собираем утилиту gifsubst `ya make $ARCADIA/smart_devices/platforms/yandexstation_2/leds/gifsubst`
2. Собираем эту утилиту `ya make $ARCADIA/alice/iot/bulbasaur/cmd/gif_generator`
3. Запускаем эту утилиту, передавая путь до gifsubst, путь до папки с шаблонами и путь до папки, в которую нужно сохранить результат

## Загружаем гифки в s3
1. Устанавливаем aws cli: https://wiki.yandex-team.ru/mds/s3-api/s3-clients/#ustanovka
2. Получаем доступы до s3. Про получение нужных прав в IDM и кредов написано тут: https://wiki.yandex-team.ru/users/skondaurov/kak-zagruzit-gifku-na-stanciju/
3. Настраиваем aws cli, используя полученные креды: https://wiki.yandex-team.ru/mds/s3-api/s3-clients/#nastrojjkaawscli.
4. Загружаем гифки температуры: `aws --endpoint-url=http://s3.mds.yandex.net s3 cp --recursive outputs/temperature/ s3://static-alice/led-production/iot/temperature`
5. Загружаем гифки процентов: `aws --endpoint-url=http://s3.mds.yandex.net s3 cp --recursive outputs/percents/ s3://static-alice/led-production/iot/percents`
