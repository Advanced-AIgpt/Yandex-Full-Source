#!/bin/bash

set -eux

script_url="$1"
req_url="$2"
python="$3"

# ===== download repo =====

curl $script_url > main.py
curl $req_url > requirements.txt
curl https://crls.yandex.net/allCAs.pem > allCAs.pem

for i in "${@:4}"
do curl -OJ "$i"
done

touch __init__.py

# ====== run script ======

export PYTHONPATH=${PYTHONPATH}:$PWD

$python -m pip install -i https://pypi.yandex-team.ru/simple --disable-pip-version-check -r requirements.txt

$python main.py
