#!/bin/bash

set -e

PIP_CMD="pip3 install -i https://pypi.yandex-team.ru/simple --disable-pip-version-check"

$PIP_CMD -U pip==9.0.1

$PIP_CMD -r requirements.txt 

# Dirty hack tornado 4.5 because of TimeoutError leak
sed -e 's@result.set_exception@if not result.done():\n            result.set_exception@' /usr/local/lib/python3.5/dist-packages/tornado/gen.py -i
