#!/bin/bash

set -e

ya package package.json --no-compression --tar --dist -E

tarfile=`python -c 'import json; print(json.load(open("packages.json"))[0]["path"])'`

./manage/manage install --yes $tarfile

rm $tarfile
rm packages.json
