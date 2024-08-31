FROM ubuntu:focal

COPY --from=registry.yandex.net/jupyter-cloud/build-image:latest /opt/venv /opt/venv
COPY includes/10_setup.sh includes/20_install.sh includes/30_cleanup.sh ./
COPY includes/scripts.sh /root/.scripts.sh

ENV PATH="/opt/venv/bin:$PATH" \
    DEBIAN_FRONTEND="noninteractive"

RUN ./10_setup.sh \
    && ./20_install.sh \
    && ./30_cleanup.sh \
    && rm *.sh

COPY includes/supervisord.conf includes/logrotate.conf /etc/
COPY includes/supervisor.logrotate /etc/logrotate.d/supervisor
COPY includes/dependent-startup.conf includes/yav-deploy.conf includes/cron.conf \
    includes/rsyslog.conf includes/unified-agent.conf \
    /etc/supervisor.d/
COPY includes/vimrc.local /etc/vim/

COPY includes/ua_config.yaml /srv/
COPY unified_agent /srv/

RUN chmod 0644 -R /etc/logrotate*

CMD ["/opt/venv/bin/supervisord", "-n", "-c", "/etc/supervisord.conf"]
