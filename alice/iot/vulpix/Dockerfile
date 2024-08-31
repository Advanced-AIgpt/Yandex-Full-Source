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
    apt-get install -o Dpkg::Options::="--force-confnew" -y nano vim telnet less curl tcpdump net-tools yandex-internal-root-ca yandex-archive-keyring yandex-ca-certs wget yandex-passport-tvmtool supervisor && \
    apt-get clean

ENV TZ Europe/Moscow
RUN echo $TZ > /etc/timezone && \
    ln -snf /usr/share/zoneinfo/$TZ /etc/localtime

COPY /bin/vulpix /usr/local/bin/vulpix
COPY misc/supervisord.conf /etc/supervisor/supervisord.conf
COPY misc/tvm.sh /usr/local/bin/tvm.sh

CMD /usr/bin/supervisord
