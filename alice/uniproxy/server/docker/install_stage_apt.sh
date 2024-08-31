#!/bin/bash

###############################################################################
#
#   Configure vars
#
UNIPROXY_WRAPPER="/usr/bin/yandex-voice-uniproxy-wrapper"
UNISTAT_WRAPPER="/usr/bin/yandex-voice-unistat-wrapper"
UNIPROXY_SUBWAY_WRAPPER="/usr/bin/yandex-voice-subway-wrapper"
UNIPROXY_INSTALL_DIR="/usr/lib/yandex/voice/uniproxy"
EXTRA_DEBUG_PACKAGES="vim mc net-tools"
UNIPROXY_FIRST_POOL="8001"
UNIPROXY_SECOND_POOL="8002"

set -e

# Comment this if you are not using dumb ipv6 configuration
sleep 30


###############################################################################
#
#   Install required packages
#
echo "UPDATING REPOS..."
apt-get update

apt-get install -y wget supervisor python3.5 python3.5-dev psmisc gcc g++ make subversion python3-pip
apt-get install -y language-pack-ru
apt-get install -y lsof strace htop
apt-get install -y perl
apt-get install -y libssl-dev
apt-get install -y libcurl4-openssl-dev
apt-get install -y iputils-ping iputils-tracepath
apt-get install -y libavfilter-ffmpeg5 libavutil-ffmpeg54 libavcodec-ffmpeg56 libavresample-ffmpeg2 libavdevice-ffmpeg56 libswscale-ffmpeg3
apt-get install -y mtr-tiny traceroute telnet tcpdump dnsutils curl iputils-ping iputils-tracepath nmap screen
apt-get install -y yandex-unbound
apt-get install -y libxml2-dev libxslt1-dev

update-alternatives --remove python3 /usr/bin/python3
update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.5 100
