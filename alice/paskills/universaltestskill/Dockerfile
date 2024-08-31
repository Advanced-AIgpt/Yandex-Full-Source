FROM registry.yandex.net/ubuntu:xenial AS base

RUN \
    apt-get update -q=2 \
    && apt-get install --assume-yes --no-install-recommends --no-install-suggests python-pip curl python-setuptools \
    && echo 'ru_RU.UTF-8 UTF-8' >> /etc/locale.gen \
    && locale-gen \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir /app
WORKDIR /app
COPY *.py *.txt /app/
RUN ls

RUN pip install -i https://pypi.yandex-team.ru/simple --upgrade pip setuptools wheel virtualenv
RUN pip install --trusted-host pypi.yandex-team.ru -i https://pypi.yandex-team.ru/simple --disable-pip-version-check -r requirements.txt

ENV FLASK_APP=test.py
CMD ["python", "main.py"]

