FROM registry.yandex.net/ubuntu:xenial

RUN sleep 60

RUN apt-get update -y
RUN apt-get install --no-install-recommends -y build-essential git python python2.7-dev\
    virtualenv vim nginx supervisor\
    htop nano screen ssh rsync psmisc less apt-utils python-setuptools curl sudo\
    aptitude tree iputils-ping

RUN locale-gen ru_RU.UTF-8
ENV LANG='ru_RU.UTF-8'
# LANGUAGE='ru_RU:ru' LC_ALL='ru_RU.UTF-8'

WORKDIR /app/

RUN virtualenv venv
RUN curl https://bootstrap.pypa.io/ez_setup.py | ./venv/bin/python
RUN ./venv/bin/pip install pip-tools

COPY logviewer/requirements.txt .
RUN ./venv/bin/pip install -r requirements.txt

COPY logviewer/logviewer/nginx.conf /etc/nginx/nginx.conf

COPY logviewer /app/
COPY utils /app/utils
RUN mkdir -p /app/public/static
RUN ./venv/bin/python manage.py collectstatic --noinput --verbosity 0

EXPOSE 80

CMD ["bash", "bootstrap.sh"]

