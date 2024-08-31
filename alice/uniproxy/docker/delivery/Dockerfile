FROM registry.yandex.net/ubuntu:xenial

ADD src/ /

RUN bash /install_stage_apt.sh

#RUN bash /install_stage_pip.sh

RUN bash /install_stage_unidelivery.sh

RUN bash /install_stage_unbound.sh

CMD [ "bash", "/docker-entry.sh" ]
