FROM registry.yandex.net/ubuntu:bionic as base

RUN apt-get update \
    && apt-get -q -y install \
        supervisor \
        tcpdump \
        openssh-client \
        yandex-openjdk15=15.0.1-tzdata2020a \
    && apt-get clean

RUN mkdir -p /dump \
    && mkdir -p /logs
WORKDIR /home/app/

FROM base

COPY skill/bilenko_skill/ app_libs/
COPY misc/ ./
COPY bilenko_skill.jar .

EXPOSE 80 8080 5005
#CMD ["./start_supervisor.sh"]
CMD ["./run_app.sh"]
