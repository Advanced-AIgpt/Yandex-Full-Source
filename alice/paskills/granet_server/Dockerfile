FROM registry.yandex.net/ubuntu:xenial as base

RUN locale-gen en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

RUN adduser granet

RUN apt-get update \
    && apt-get -q -y install \
        yandex-push-client \
        tcpdump \
        jq \
        openssh-client \
    && apt-get clean

WORKDIR /home/granet

COPY ./server /home/granet/server
COPY /logrotate /home/granet/logrotate
COPY /nanny /home/granet/nanny
COPY config /home/granet/config

RUN chmod +x /home/granet/nanny/*.sh
RUN chown 0640 /home/granet/nanny/logrotate.config

EXPOSE 80
CMD ["/home/granet/nanny/run_granet_server", "--config", "config/testing.pb.txt"]
