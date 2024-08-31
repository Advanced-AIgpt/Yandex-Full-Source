FROM registry.yandex.net/ubuntu:xenial AS base

RUN \
    apt-get update -q=2 \
    && apt-get install --assume-yes --no-install-recommends --no-install-suggests python-pip curl python-setuptools \
    && echo 'ru_RU.UTF-8 UTF-8' >> /etc/locale.gen \
    && locale-gen \
    && rm -rf /var/lib/apt/lists/*


RUN mkdir /app /app/lib
ENV PYTHONPATH=/app/binary/lemmer/linux/
WORKDIR /app

COPY requirements.txt .
RUN pip install -i https://pypi.yandex-team.ru/simple --upgrade pip setuptools wheel virtualenv
RUN pip install --trusted-host pypi.yandex-team.ru -i https://pypi.yandex-team.ru/simple --disable-pip-version-check -r requirements.txt

# copy binary libs
COPY binary/lemmer/linux/liblemmer_python_binding.so /app/binary/lemmer/linux/liblemmer_python_binding.so
COPY binary/reverse-normalizer/ru/* /app/binary/reverse-normalizer/ru/

# copy sources
COPY nlu_service /app/nlu_service
COPY cli.py /app
COPY setup.py /app/setup.py
RUN python setup.py install


FROM base AS tests
COPY requirements-dev.txt pytest.ini /app/
COPY tests /app/tests
RUN pip install --trusted-host pypi.yandex-team.ru -i https://pypi.yandex-team.ru/simple --disable-pip-version-check -r requirements-dev.txt
RUN pytest


FROM base AS service
CMD ["python", "cli.py", "serve"]