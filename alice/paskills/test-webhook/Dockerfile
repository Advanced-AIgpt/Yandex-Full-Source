FROM registry.yandex.net/ubuntu:xenial as base

RUN locale-gen en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

RUN adduser webhook

WORKDIR /home/webhook

COPY /integration /home/webhook/integration
COPY /dialogovo_webhook /home/webhook/dialogovo_webhook

CMD ["/home/webhook/dialogovo_webhook", "--dir", "/home/webhook/integration"]

EXPOSE 80

