#!/bin/bash

set -e

echo "deb http://dist.yandex.ru/yandex-voicetech-trusty stable/amd64/"  >> /etc/apt/sources.list.d/yandex.list
echo "deb http://search.dist.yandex.ru/search/ stable/all/"             >> /etc/apt/sources.list.d/yandex.list
echo "deb http://search.dist.yandex.ru/search/ stable/amd64/"           >> /etc/apt/sources.list.d/yandex.list
echo "deb http://search.dist.yandex.ru/search/ unstable/all/"           >> /etc/apt/sources.list.d/yandex.list
echo "deb http://search.dist.yandex.ru/search/ unstable/amd64/"         >> /etc/apt/sources.list.d/yandex.list

apt-get update
apt-get install -y --force-yes \
    g++=4:4.8.2-1ubuntu6 \
    gcc=4:4.8.2-1ubuntu6 \
    gfortran=4:4.8.2-1ubuntu6 \
    libatlas-base-dev=3.10.1-4 \
    libatlas-dev=3.10.1-4 \
    libblas-dev=1.2.20110419-7 \
    liblapack-dev=3.5.0-2ubuntu1 \
    libboost-python-dev=1.54.0.1ubuntu1 \
    libxml2-dev=2.9.1+dfsg1-3ubuntu4.13 \
    libxslt1-dev=1:1.1.26-1yandex6 \
    python-dev=2.7.5-5ubuntu3 \
    python-pip=8.1.2-yandex1 \
    python-setuptools=3.3-1ubuntu2 \
    yandex-voice-openfst=0.0.1 \
    zlib1g-dev=1:1.2.8.dfsg-1ubuntu1 \
    supervisor=3.0b2-1 \
    mongodb=1:3.2.12-yandex1 \
    mongodb-mongos=1:3.2.12-yandex1 \
    mongodb-server=1:3.2.12-yandex1 \
    mongodb-shell=1:3.2.12-yandex1 \
    mongodb-tools=1:3.2.12-yandex1 \
    mongodb-user=1:3.2.12-yandex1 \
    config-monrun-mongodb-basecheck-python=1.82 \
    redis-server4=4.0.2-yandex1 \
    unzip

apt-get clean
rm -rf /var/lib/apt/lists/*

pip install -i https://pypi.yandex-team.ru/simple/ -U pip

# install protoc compiler
PROTOC_ZIP=protoc-3.3.0-linux-x86_64.zip
curl -OL https://github.com/google/protobuf/releases/download/v3.3.0/$PROTOC_ZIP
unzip -o $PROTOC_ZIP -d /usr/local bin/protoc
rm -f $PROTOC_ZIP
