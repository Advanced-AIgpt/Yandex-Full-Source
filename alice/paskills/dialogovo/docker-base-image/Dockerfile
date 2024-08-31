FROM registry.yandex.net/rtc-base/xenial:stable

RUN locale-gen en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

COPY root.crt /root/.postgresql/root.crt

RUN wget --quiet -O - https://www.postgresql.org/media/keys/ACCC4CF8.asc | apt-key add - \
    && echo "deb http://apt.postgresql.org/pub/repos/apt/ xenial-pgdg main" > /etc/apt/sources.list.d/pgdg_xenial.list \
    && apt-get update \
    && apt-get -q -y install \
        postgresql-client-10 \
        yandex-passport-tvmtool \
        yandex-push-client \
        tcpdump \
        telnet \
        jq \
        openssh-client \
    && apt-get clean

RUN mkdir -p /var/log/yandex/dialogovo/heapdump \
    && mkdir -p /logs \
    && mkdir -p /dump


RUN adduser app
RUN chown app /var/log/yandex/dialogovo/heapdump
RUN chown app /logs
RUN chown app /dump
USER app
WORKDIR /home/app/
