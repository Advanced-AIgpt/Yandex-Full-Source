FROM registry.yandex.net/paskills/quasar-billing-base-image:8965230 as base

WORKDIR /home/app/

RUN mkdir -p jdk
COPY jdk/ jdk

COPY /misc/supervisord.conf /etc/supervisor/supervisord.conf
COPY /misc/tvm.sh /usr/local/bin/tvm.sh
COPY /misc/tvm.prod.json misc/tvm.test.json /usr/local/etc/
COPY /misc/run.sh run.sh
COPY /misc/shutdown.sh shutdown.sh
RUN chmod +x run.sh
RUN chmod +x run.sh

COPY misc/push-client.sh /usr/local/bin/push-client.sh
COPY misc/push-client.prod.yaml misc/push-client.test.yaml /usr/local/etc/

COPY /configs/test test
COPY /configs/prod prod
COPY /configs/java java

COPY /lib/quasar-billing/ /home/app/lib
COPY /quasar-billing.jar /home/app/quasar-billing.jar

EXPOSE 80 8080 5005
CMD ["supervisord", "-c", "/etc/supervisor/supervisord.conf"]

#for profiling
#ssh -L 1089:localhost:1089 -L 1090:localhost:1090 <host as for ssh>
#EXPOSE 1089
