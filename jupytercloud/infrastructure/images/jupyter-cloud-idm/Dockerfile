FROM registry.yandex.net/jupyter-cloud/deploy-app:latest

ENV PATH="/opt/venv/bin:$PATH"

COPY includes/jupyter-cloud-idm.conf /etc/supervisor.d/
COPY includes/jupyter-cloud-idm.logrotate /etc/logrotate.d/jupyter-cloud-idm
COPY includes/yav-deploy /srv/yav-deploy
COPY includes/ua_config.yaml /srv/

COPY jupytercloud /srv/

RUN mkdir -p /var/log/jupyter-cloud-idm \
    && chmod 644 /etc/logrotate.d/*
