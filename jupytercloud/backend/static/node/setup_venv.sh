#!/bin/bash

set -e;
set -x;

dir=$(dirname "$0");

if [ -d "${dir}/venv" ]; then
	exit 0;
fi;

echo 1;

env python3 -m virtualenv venv;

pip="${dir}/venv/bin/pip";

${pip} install yandex-passport-vault-client boto3 -i https://pypi.yandex-team.ru/simple;
