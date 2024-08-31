FROM registry.yandex.net/jupyter-cloud/deploy-app:latest

ARG SALT_VERSION=3004
ENV PATH="/opt/venv/bin:$PATH"

RUN pip3.9 install --no-cache-dir -U -i https://pypi.yandex-team.ru/simple \
        salt=="${SALT_VERSION}" CherryPy ws4py psutil psycopg2-binary 'Jinja2<3.1' \
        sandbox-library \
    && find /opt \
        -type f -a \( -name '*.pyc' -o -name '*.pyo' -o -name '*.a' \) -delete

COPY includes/key-accepter.conf \
    includes/salt-api.conf \
    includes/salt-master.conf /etc/supervisor.d/
COPY includes/salt.logrotate /etc/logrotate.d/salt-common
COPY includes/key-accepter.logrotate /etc/logrotate.d/key-accepter
COPY includes/yav-deploy /srv/yav-deploy

COPY jupytercloud /srv/
COPY salt/master /srv/master
COPY salt/states /srv/states
COPY salt/pillar /srv/pillar
COPY salt/reactor /srv/reactor
COPY salt/modules /srv/modules
COPY salt/extensions /srv/extensions

RUN mkdir -p /var/log/local-key-accepter \
    && mkdir -p /var/log/salt \
    && rm -rf /etc/salt && ln -sT /srv /etc/salt \
    && chmod 0644 -R /etc/logrotate*
