FROM registry.yandex.net/ubuntu:xenial as base

RUN locale-gen en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

COPY /config.json /config.json
COPY /billing_fake_ott_api /billing_fake_ott_api

CMD ["/billing_fake_ott_api", "--config", "/config.json"]

EXPOSE 8080
EXPOSE 80
