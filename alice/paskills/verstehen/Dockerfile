FROM registry.yandex.net/ubuntu:xenial AS base

COPY verstehen /verstehen
COPY config.json /config.json
COPY granet_ui /granet_ui

expose 80
CMD ["/bin/bash"]
