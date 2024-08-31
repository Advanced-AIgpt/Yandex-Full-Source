#!/bin/bash

set -ex

target=$1

root_dir=$(readlink -f $(dirname "${BASH_SOURCE[0]}")/../../)
data_dir=$(readlink -f $root_dir"/data/load_testing")

function run_tank() {
    config=$1
    ammo=$2
    yandex-tank -c $data_dir/$config $data_dir/$ammo
}

case $target in
    navi_app_tst)
        run_tank navi_app_tst.ini navi_app.ammo;;
    navi_app_dev)
        run_tank navi_app_dev.ini navi_app.ammo;;
    navi_app_dev_long)
        run_tank navi_app_dev_long.ini navi_app.ammo;;
    navi_app_prod)
        run_tank navi_app_prod.ini navi_app.ammo;;
    wizard_tst)
        run_tank wizard_tst.ini wizard_tst.ammo;;
    wizard_prod)
        run_tank wizard_prod.ini wizard_prod.ammo;;
    geo_load)
        run_tank geosearch_load.ini geosearch_tst.ammo;;
    *)
        echo "No configuration for target '$target' found"
    ;;
esac

