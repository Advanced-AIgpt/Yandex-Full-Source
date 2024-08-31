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
        supervisor \
        tcpdump \
        telnet \
        jq \
        openssh-client \
        linux-tools-generic \
        linux-tools-common \
    && apt-get clean

RUN mkdir -p /var/log/yandex/heapdump \
    && mkdir -p /logs \
    && mkdir -p /dump

# copied from https://github.yandex-team.ru/recommender/zen/blob/master/zen/src/docker/yandex-ichwill-zen-base.dockerfile
RUN mkdir -p /usr/local/async_profiler && \
    curl -s -L -o /usr/local/async_profiler/async_profiler.tar.gz https://proxy.sandbox.yandex-team.ru/1146553733 && \
    tar xzf /usr/local/async_profiler/async_profiler.tar.gz -C /usr/local/async_profiler && \
    rm /usr/local/async_profiler/async_profiler.tar.gz

RUN mkdir -p /usr/local && \
    curl -s -L -o /usr/local/perf_map.tar.gz https://proxy.sandbox.yandex-team.ru/1146466431 && \
    tar xzf /usr/local/perf_map.tar.gz -C /usr/local && \
    rm /usr/local/perf_map.tar.gz

ENV FLAMEGRAPH_DIR /usr/local/FlameGraph
