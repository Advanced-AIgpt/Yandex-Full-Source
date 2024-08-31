FROM registry.yandex.net/jupyter-cloud/deploy-app:latest

ENV PATH="/opt/venv/bin:$PATH"

ARG traefik_url="https://github.com/traefik/traefik/releases/download/v2.4.8/traefik_v2.4.8_linux_amd64.tar.gz"
ARG traefik_file="traefik_v2.4.8_linux_amd64.tar.gz"
ARG traefik_sha="de8d56f6777c5098834d4f8d9ed419b7353a3afe913393a55b6fd14779564129"

RUN wget -q ${traefik_url} \
    && echo "${traefik_sha}  ${traefik_file}" | sha256sum -c - \
    && tar xf ${traefik_file} \
    && rm ${traefik_file} \
    && mv traefik /srv

COPY includes/traefik.conf includes/endpoint-generator.conf /etc/supervisor.d/
COPY includes/redis_endpoint_generator_config.py /srv/
COPY includes/yav-deploy /srv/yav-deploy
COPY includes/proxy.logrotate /etc/logrotate.d/proxy
COPY includes/ua_config.yaml /srv/

COPY jupytercloud /srv/

RUN chmod 644 /etc/logrotate.d/* \
    && mkdir /var/log/traefik \
    && mkdir /var/log/endpoint_generator

