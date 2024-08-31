# TODO: move to general image
FROM registry.yandex.net/paskills/jvm-base-image:7721987

COPY misc/supervisord.conf /etc/supervisor/supervisord.conf
COPY misc/run.sh run.sh
COPY java/* java/
COPY config/* config/

COPY lib/my_alice_pumpkin/ /home/app/lib

COPY my_alice_pumpkin.jar /home/app/my_alice_pumpkin.jar

EXPOSE 5005 10000
