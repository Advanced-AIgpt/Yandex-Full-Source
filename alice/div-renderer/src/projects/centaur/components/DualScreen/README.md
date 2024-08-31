Ставим все что надо для начала работы с проектом (ридми в корне)

Подключаем устройство по `adb`, разрешаем usb отладку и запускаем команду ниже

```shell
node ../../../../../dist/projects/centaur/components/DualScreen/example/index.js &&  ../../../../../../../../smart_devices/tools/centaur_test/centaur_test execute --name show_view --payload ../../../../../dist/projects/centaur/components/DualScreen/example/test.json && cat ../../../../../dist/projects/centaur/components/DualScreen/example/test.json && rm ../../../../../dist/projects/centaur/components/DualScreen/example/test.json
```
