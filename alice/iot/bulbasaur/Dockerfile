FROM registry.yandex.net/rtc-base/bionic:stable

RUN apt-get update -qq && \
    apt-get install -y gnupg && \
    apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 7FCD11186050CD1A

RUN echo "deb http://dist.yandex.ru/common stable/all/" >> /etc/apt/sources.list && \
    echo "deb http://dist.yandex.ru/common stable/amd64/" >> /etc/apt/sources.list && \
    echo "deb http://dist.yandex.ru/search stable/all/" >> /etc/apt/sources.list && \
    echo "deb http://dist.yandex.ru/search stable/amd64/" >> /etc/apt/sources.list && \
    echo "deb http://dist.yandex.ru/yandex-bionic/ stable/all/" >> /etc/apt/sources.list && \
    echo "deb http://dist.yandex.ru/yandex-bionic/ stable/amd64/" >> /etc/apt/sources.list

RUN apt-get update -qq && \
    apt-get install -o Dpkg::Options::="--force-confnew" -y nano vim telnet less curl tcpdump net-tools yandex-internal-root-ca yandex-archive-keyring yandex-ca-certs wget yandex-passport-tvmtool yandex-push-client supervisor logrotate rsync yandex-unified-agent && \
    apt-get clean

RUN apt-get -y install tzdata

ENV TZ Europe/Moscow
RUN echo $TZ > /etc/timezone && \
    ln -snf /usr/share/zoneinfo/$TZ /etc/localtime

COPY misc/supervisord.conf /etc/supervisor/supervisord.conf
RUN mkdir -p /logs
RUN touch /logs/supervisord.log

RUN mkdir -p /pdata/unified-agent  # storage and cache for unified agent

COPY misc/tvm.sh /usr/local/bin/tvm.sh

COPY misc/push-client/push-client.sh /usr/local/bin/push-client.sh
COPY misc/push-client/push-client.conf.production misc/push-client/push-client.conf.beta misc/push-client/push-client.conf.dev /usr/local/etc/

COPY misc/logrotate/logrotate.sh /usr/local/bin/logrotate.sh
COPY misc/logrotate/logrotate.conf /usr/local/etc/

COPY misc/unified-agent/unified-agent.sh /usr/local/bin/unified-agent.sh
COPY misc/unified-agent/unified-agent.production.yaml misc/unified-agent/unified-agent.beta.yaml /usr/local/etc/

COPY /config/*.yaml /etc/bulbasaur.conf/
COPY /bin/bulbasaur /usr/local/bin/bulbasaur

CMD /usr/bin/supervisord
