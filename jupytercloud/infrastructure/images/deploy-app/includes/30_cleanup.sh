apt-get --autoremove --yes purge \
    gnupg apt-transport-https apt-utils
apt-get --yes clean

rm -rf /tmp/* \
    /var/tmp/* \
    /var/lib/apt/lists/* \
    /var/lib/cache/* \
    /usr/share/doc/*

find / \
    -type f -a \( -name '*.pyc' -o -name '*__pycache__*' \) -delete

find /usr/lib/x86_64-linux-gnu/gconv \
    -type f -a \! \( -name 'UTF-*' -o -name 'UNICODE*' -o -name "gconv*" \) -delete

locale-gen --purge
