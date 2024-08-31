
apt-get update

apt-get install -y \
    supervisor \
    psmisc \
    iputils-ping \
    iputils-tracepath \
    mtr-tiny \
    traceroute \
    telnet \
    tcpdump \
    dnsutils \
    curl \
    iputils-ping \
    iputils-tracepath \
    nmap \
    screen \
    tmux \
    yandex-unbound


# ====================================================================================================================
#    UNBOUND CONFIG
# ====================================================================================================================

cat << EOF > /etc/unbound/unbound.conf
server:
    logfile: /logs/unbound.out
    username: loadbase
    module-config: "iterator"
    interface: 127.0.0.1
    interface: ::1
    access-control: 127.0.0.0/8 allow_snoop
    access-control: ::1/128 allow_snoop
    prefetch: yes
    so-reuseport: yes
    local-zone: "localhost.yandex.ru." static
    local-data: "localhost.yandex.ru. 10800 IN NS localhost."
    local-data: "localhost.yandex.ru. 10800 IN A 127.0.0.1"
    local-data: "localhost.yandex.ru. 10800 IN AAAA ::1"
    local-zone: "localhost.yandex.net." static
    local-data: "localhost.yandex.net. 10800 IN NS localhost."
    local-data: "localhost.yandex.net. 10800 IN A 127.0.0.1"
    local-data: "localhost.yandex.net. 10800 IN AAAA ::1"

remote-control:
    control-enable: yes

forward-zone:
    name: "."
    forward-addr: 2a02:6b8:0:3400::5005
EOF


cat << EOF > /etc/resolv.conf
nameserver ::1
nameserver 2a02:6b8:0:3400::5005
search yandex.net yandex.ru
options timeout:1 attempts:1
EOF


echo "UTC" > /etc/timezone
ln -sf /usr/share/zoneinfo/UTC /etc/localtime


apt-get autoremove -y --force-yes
apt-get clean

chown loadbase:loadbase -R /etc/unbound/

rm -rf /var/lib/apt
rm -rf /var/cache

rm -rf /tmp/*
