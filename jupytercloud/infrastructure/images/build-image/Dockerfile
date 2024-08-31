FROM registry.yandex.net/rtc-base/focal:stable AS build-image

ENV DEBIAN_FRONTEND=noninteractive
ENV PATH="/opt/venv/bin:$PATH"

RUN apt-get update --yes \
    # Build + APT tools
    && apt-get install --yes --no-install-recommends --no-install-suggests \
        build-essential gcc software-properties-common \
    && wget "https://crls.yandex.net/allCAs.pem" -qO /srv/allCAs.pem \
    && add-apt-repository --yes --update ppa:deadsnakes/ppa \
    #
    # Install Python3.9 so it's same in build and base images
    && apt-get install --yes --no-install-recommends --no-install-suggests \
        python3.9 python3.9-dev python3.9-venv python3.9-distutils \
    && wget -q -O - https://bootstrap.pypa.io/get-pip.py | python3.9 \
    && python3.9 -m venv /opt/venv \
    && python3.9 -m pip install --no-cache-dir --upgrade \
        pip setuptools wheel \
    #
    # PIP packages which require GCC for later images
    && python3.9 -m pip install --no-cache-dir --upgrade \
        timelib setproctitle \
    #
    # Small cleanup
    && apt-get purge --autoremove --yes \
        build-essential gcc \
    && apt-get clean --yes \
    && find /opt \
        -type f -a \( -name '*.pyc' -o -name '*.pyo' -o -name '*.a' \) -delete \
    && rm -rf /var/lib/apt/lists/* \
        /var/lib/cache/* \
        /tmp/* \
        /var/tmp/* \
        /var/lib/apt/lists/* \
        /var/lib/cache/* \
        /var/log/* \
        /usr/share/doc/*
