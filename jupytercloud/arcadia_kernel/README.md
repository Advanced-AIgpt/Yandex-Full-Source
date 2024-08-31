Сборка пакета с ядрами
======================

Необходимо выполнить команду: `ya package package.json -r --dist`

`-r` собирает релизную сборку.

`--dist` делает это на distbuild, а не локально.

На выходе получаем файл `jupyter-cloud-jupyter-kernel.<revision>.tar.gz`


Установка пакета с ядрами в систему
===================================

`./manage/manage.py install jupyter-cloud-jupyter-kernel.<revision>.tar.gz`

Можно управлять местом, куда будут установлены ядра, см. `./manage/manage.py -h`.

Быстрая сборка пакета и установка ядер для разработки: `./build_install.sh`
