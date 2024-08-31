#!/bin/sh

set -xe

# compile protocol buffers
protoc -I=core/vins_core/schema/protofiles/ --python_out=core/vins_core/schema/ \
    core/vins_core/schema/protofiles/features.proto

PIP_INSTALL="pip install --disable-pip-version-check --no-cache-dir -i https://pypi.yandex-team.ru/simple/"

for lib in $*
do
    $PIP_INSTALL -r $lib/requirements.txt
    $PIP_INSTALL -e $lib/
done
