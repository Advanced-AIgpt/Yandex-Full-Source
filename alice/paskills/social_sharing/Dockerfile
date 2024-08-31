FROM registry.yandex.net/paskills/jvm-base-image:7721987

COPY misc/shutdown.sh shutdown.sh
RUN mkdir -p java
COPY java/* java/

COPY log4j2.xml log4j2.xml
COPY lib/social_sharing/ /home/app/lib
COPY social_sharing.jar /home/app/social_sharing.jar

EXPOSE 80 5005 10000
