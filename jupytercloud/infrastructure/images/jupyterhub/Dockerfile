FROM registry.yandex.net/jupyter-cloud/deploy-app:latest

ENV PATH="/opt/venv/bin:$PATH"

RUN apt-get update \
    && apt-get install --assume-yes --no-install-recommends --no-install-suggests \
       libcurl4-openssl-dev subversion \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

COPY includes/jupyterhub.conf includes/backend-switcher.conf /etc/supervisor.d/
COPY includes/jupyterhub.logrotate /etc/logrotate.d/jupyterhub
COPY includes/services.logrotate /etc/logrotate.d/services
COPY includes/yav-deploy /srv/yav-deploy
COPY includes/ua_config.yaml includes/ua_service_config.yaml /srv/

COPY jupytercloud /srv/

RUN mkdir -p /var/log/jupyterhub \
        /var/log/jupytercloud-dns-sync \
        /var/log/consistency-watcher \
        /var/log/backend-switcher \
    && chmod -R 0600 /var/log/jupyterhub \
        /var/log/jupytercloud-dns-sync \
        /var/log/consistency-watcher \
        /var/log/backend-switcher \
    && chmod 644 /etc/logrotate.d/*

