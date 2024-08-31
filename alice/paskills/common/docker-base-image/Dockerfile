FROM registry.yandex.net/rtc-base/xenial:stable as base

RUN apt-get update \
    && apt-get -q -y install \
        yandex-passport-tvmtool \
        yandex-unified-agent \
        supervisor \
        tcpdump \
        telnet \
        jq \
        openssh-client \
        yandex-openjdk11=11.0.8-tzdata2020e \
    && apt-get clean

RUN mkdir -p /dump \
    && mkdir -p /logs
WORKDIR /home/app/
