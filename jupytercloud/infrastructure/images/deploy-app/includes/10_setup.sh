#!/usr/bin/env bash
set -euxo pipefail

tee etc/resolv.conf <<EOF
nameserver 2a02:6b8:0:3400::5005
nameserver 2a02:6b8:0:3400::1
nameserver 2a02:6b8::1:1
options timeout:1 attempts:1
EOF

tee -a /etc/apt/apt.conf.d/recommends-suggests << EOF
APT::Install-Recommends "0" ;
APT::Install-Suggests "0" ;
EOF

tee /etc/apt/sources.list.d/yandex-common.list <<EOF
deb [trusted=yes] http://common.dist.yandex.ru/common stable/all/
deb [trusted=yes] http://common.dist.yandex.ru/common stable/amd64/
EOF


apt-get update
apt-get install --yes \
    apt-utils software-properties-common
add-apt-repository ppa:deadsnakes/ppa
apt-get install --yes \
    tzdata locales locales-all gnupg2 yandex-archive-keyring
gpg --keyserver keyserver.ubuntu.com --recv-keys 7FCD11186050CD1A

tee /etc/apt/sources.list.d/yandex-focal.list <<EOF
deb http://search-focal.dist.yandex.ru/search-focal stable/all/
deb http://search-focal.dist.yandex.ru/search-focal stable/amd64/
deb http://search-focal.dist.yandex.ru/search-focal unstable/all/
deb http://search-focal.dist.yandex.ru/search-focal unstable/amd64/

deb http://yandex-focal.dist.yandex.ru/yandex-focal stable/all/
deb http://yandex-focal.dist.yandex.ru/yandex-focal stable/amd64/
deb http://yandex-focal.dist.yandex.ru/yandex-focal unstable/all/
deb http://yandex-focal.dist.yandex.ru/yandex-focal unstable/amd64/

deb http://search-upstream-focal.dist.yandex.ru/search-upstream-focal stable/all/
deb http://search-upstream-focal.dist.yandex.ru/search-upstream-focal stable/amd64/
deb http://search-upstream-focal.dist.yandex.ru/search-upstream-focal unstable/all/
deb http://search-upstream-focal.dist.yandex.ru/search-upstream-focal unstable/amd64/
EOF

echo "Europe/Moscow" > /etc/timezone
echo "LC_ALL=en_US.UTF-8" >> /etc/environment
echo "LANG=en_US.UTF-8" >> /etc/environment
locale-gen en_US.UTF-8
update-locale en_US.UTF-8
ln -fs /usr/share/zoneinfo/Europe/Moscow /etc/localtime
dpkg-reconfigure -f noninteractive tzdata
dpkg-reconfigure -f noninteractive locales

ln -sf /dev/null /etc/systemd/system/apt-daily-upgrade.service
ln -sf /dev/null /etc/systemd/system/apt-daily.service
