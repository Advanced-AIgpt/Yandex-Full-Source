FROM registry.yandex.net/ubuntu:xenial

ADD docker/install_stage_apt.sh /
RUN bash /install_stage_apt.sh

ADD requirements.txt /
ADD docker/install_stage_pip.sh /
RUN bash install_stage_pip.sh

ADD _vimrc /root/.vimrc
ADD protoc /usr/bin/

ADD src /usr/lib/yandex/voice/uniproxy/
ADD docker/install_stage_uniproxy.sh /
RUN bash /install_stage_uniproxy.sh

ADD docker/install_stage_unbound.sh /
RUN bash install_stage_unbound.sh

ADD docker-entry.sh /

CMD [ "bash", "/docker-entry.sh" ]
