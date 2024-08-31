FROM ubuntu:bionic

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
    apt-get install -o Dpkg::Options::="--force-confnew" -y nano vim telnet less curl tcpdump net-tools yandex-internal-root-ca yandex-archive-keyring yandex-ca-certs wget yandex-passport-tvmtool supervisor rsync && \
    apt-get clean

ENV TZ Europe/Moscow
RUN echo $TZ > /etc/timezone && \
    ln -snf /usr/share/zoneinfo/$TZ /etc/localtime

COPY /misc/supervisord.conf /etc/supervisor/supervisord.conf
COPY /config/*.yaml /etc/trigger/

COPY /bin/server /usr/local/bin/

CMD /usr/bin/supervisord -c /etc/supervisor/supervisord.conf
