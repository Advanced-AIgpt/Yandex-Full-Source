# TODO: move to general image
FROM registry.yandex.net/paskills/jvm-base-image:7721987

COPY misc/supervisord.conf /etc/supervisor/supervisord.conf
COPY misc/run.sh run.sh
COPY java/* java/
COPY config/* config/

COPY lib/my_alice/ /home/app/lib
COPY geobase /home/app/geobase
RUN tar -xvf /home/app/geobase/data/tzdata/tzdata.tar.gz -C /home/app/geobase/data/tzdata

COPY my_alice.jar /home/app/my_alice.jar

EXPOSE 5005 10000

# ENV commands are not supported in RTC, this variable will be used only in local docker containers
ENV GEOBASE_TZ_PATH=/home/app/geobase/data/tzdata/zones_bin

CMD ["supervisord", "-c", "/etc/supervisor/supervisord.conf"]
