#FROM registry.yandex.net/paskills/jvm-base-image:7328425
FROM registry.yandex.net/rtc-base/bionic:stable as base

RUN apt-get update \
    && apt-get -q -y install \
        yandex-passport-tvmtool \
        yandex-unified-agent \
        supervisor \
        tcpdump \
        telnet \
        jq \
        openssh-client \
    && apt-get clean

RUN mkdir -p /dump \
    && mkdir -p /logs
WORKDIR /home/app/

FROM base

RUN mkdir -p jdk
COPY jdk/ jdk
COPY memento/memento/ app_libs/
COPY misc/ ./
COPY arcadia_libs/ app_libs/
COPY config/ config/
COPY memento.jar .

EXPOSE 80 8080 5005
CMD ["./start_supervisor.sh"]
