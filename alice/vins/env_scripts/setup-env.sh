#!/bin/bash

set -ex

apt-get update
apt-get install -y --force-yes \
        python-software-properties \
        software-properties-common \


UBUNTU_VERSION='trusty'


APT_SOURCES="/etc/apt/sources.list.d/yandex.list"
APT_SOURCES_ALL="/etc/apt/sources.list.d/*.list"
DIST="http://dist.yandex.ru"

REPO="$DIST/yandex-$UBUNTU_VERSION"
if ! grep -q $REPO $APT_SOURCES_ALL; then
    echo "deb $REPO stable/all/" >> $APT_SOURCES
    echo "deb $REPO stable/amd64/" >> $APT_SOURCES
fi

REPO="$DIST/yandex-voicetech-$UBUNTU_VERSION"
if ! grep -q $REPO $APT_SOURCES_ALL; then
    echo "deb $REPO stable/all/" >> $APT_SOURCES
    echo "deb $REPO stable/amd64/" >> $APT_SOURCES
    echo "deb $REPO unstable/all/" >> $APT_SOURCES
    echo "deb $REPO unstable/amd64/" >> $APT_SOURCES
fi

REPO="$DIST/common"
if ! grep -q $REPO $APT_SOURCES_ALL; then
    echo "deb $REPO stable/all/" >> $APT_SOURCES
    echo "deb $REPO stable/amd64/" >> $APT_SOURCES
fi

REPO="$DIST/yc2-staging"
if ! grep -q $REPO $APT_SOURCES_ALL; then
    echo "deb $REPO precise-cloud-tools/all/" >> $APT_SOURCES
    echo "deb $REPO precise-cloud-tools/amd64/" >> $APT_SOURCES
fi

apt-get update
apt-get install -y --force-yes yandex-archive-keyring

apt-get update
apt-get install -y --force-yes \
        libblas-dev libatlas-dev liblapack-dev libtcmalloc-minimal4 \
        libboost-python-dev gcc g++ subversion \
        build-essential gfortran libatlas-base-dev \
        libxml2-dev libxslt-dev  \
        git-core zlib1g-dev mongodb \
        yandex-voice-openfst python-dev \
        python-virtualenv python-setuptools \
        python-wheel python-pip \
        libgeobase5-python \
        unzip redis-server4 \
        libncurses5-dev

# install protoc compiler
PROTOC_ZIP=protoc-3.3.0-linux-x86_64.zip
curl -OL https://github.com/google/protobuf/releases/download/v3.3.0/$PROTOC_ZIP
unzip -o $PROTOC_ZIP -d /usr/local bin/protoc
rm -f $PROTOC_ZIP
