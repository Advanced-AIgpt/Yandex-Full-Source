FROM registry.yandex.net/paskills/dialogovo-base-image:9063036 as base

RUN mkdir -p jdk
COPY jdk/ jdk

COPY misc/tvm.sh /usr/local/bin/tvm.sh
COPY misc/tvm.prod.json misc/tvm.priemka.json misc/tvm.test.json /usr/local/etc/
COPY misc/run.sh run.sh
COPY misc/shutdown.sh shutdown.sh
COPY misc/dialogovo_start.sh dialogovo_start.sh

COPY misc/push-client.sh /usr/local/bin/push-client.sh
COPY misc/push-client.prod.yaml misc/push-client.priemka.yaml misc/push-client.test.yaml /usr/local/etc/

COPY config/test test
COPY config/prod prod
COPY config/priemka priemka
COPY lib/kronstadt-shard-runner/ /home/app/lib
COPY lib/dialogovo/ /home/app/lib

COPY dialogovo.jar /home/app/dialogovo.jar

EXPOSE 80 8080 5005
