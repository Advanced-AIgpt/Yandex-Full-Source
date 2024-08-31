FROM registry.yandex.net/ubuntu:xenial

RUN apt-get update -y && \
    apt-get install -y -q0 --no-install-recommends && \
    apt-get install -y \
        supervisor \
        yandex-push-client \
        redis-server

COPY app /app
COPY dssm_model /dssm_model
COPY configs /configs
COPY configs/supervisord.conf /etc/supervisor/conf.d/supervisord.conf

RUN mkdir /logs

EXPOSE 80
CMD /usr/bin/supervisord
