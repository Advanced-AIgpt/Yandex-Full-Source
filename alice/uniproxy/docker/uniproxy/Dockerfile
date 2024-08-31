FROM registry.yandex.net/ubuntu:xenial

ADD src/ /
#ADD src/install_stage_apt.sh src/install_stage_pip.sh src/requirements.txt /

RUN bash /install_stage_apt.sh

#RUN bash /install_stage_pip.sh

#RUN bash /integrate_arcadia_packages.sh

RUN bash /install_stage_uniproxy.sh

RUN bash /install_stage_unbound.sh

CMD [ "bash", "/docker-entry.sh" ]
