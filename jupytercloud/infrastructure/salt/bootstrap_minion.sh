#!/bin/bash

set -e

if [ -z $ENV_NAME ]
then
	echo "Empty variable ENV_NAME";
	exit 1;
fi

if [ $ENV_NAME == "production" ]
then
	master_host="
    - salt-iva.jupyter.yandex-team.ru
    - salt-myt.jupyter.yandex-team.ru
    - salt-sas.jupyter.yandex-team.ru"
elif [ $ENV_NAME == "development" ]
then
	master_host="
    - salt-iva.beta.jupyter.yandex-team.ru
    - salt-myt.beta.jupyter.yandex-team.ru
    - salt-sas.beta.jupyter.yandex-team.ru"
else
	echo "Wrong value for ENV_NAME";
	exit 1;
fi

# to prevent echo-command from logging we wrap command with {set +x; ...; set -x} 2>/dev/null
# because bootstrap script itself invoked with `bash -x`
{
	set +x;
	if [ -z "$SSH_KEY" ]
	then
		echo "Empty variable SSH_KEY";
		exit 1;
	else
		echo "$SSH_KEY" > /srv/ssh.key;
		chmod 0600 /srv/ssh.key;
	fi
	set -x;
} 2>/dev/null

SVN_SSH="ssh -v -i /srv/ssh.key -o StrictHostKeyChecking=no"
export SVN_SSH
svn co svn+ssh://robot-jupyter-cloud@arcadia.yandex.ru/arc/trunk/arcadia/jupytercloud/infrastructure/salt/states /srv/states
svn co svn+ssh://robot-jupyter-cloud@arcadia.yandex.ru/arc/trunk/arcadia/jupytercloud/infrastructure/salt/extensions /srv/extensions
svn co svn+ssh://robot-jupyter-cloud@arcadia.yandex.ru/arc/trunk/arcadia/jupytercloud/infrastructure/salt/pillar /srv/pillar
rm /srv/ssh.key

# wait until systemd is completely loaded
# sleep 600

# it prevents flapping error when this service makes something and our apt-get failing because of it
systemctl mask apt-daily.service || true;
systemctl mask apt-daily-upgrade.service || true;
systemctl kill apt-daily.service || true;
systemctl kill apt-daily-upgrade.service || true;

export DEBIAN_FRONTEND=noninteractive

apt-get update
# it allows to use ipv4-only services inside bootstrap script

file /etc/resolv.conf;
# apt-get install -y yandex-config-dns64 && break || sleep 60;
tee <<EOF /etc/resolv.conf
nameserver 2a02:6b8:0:3400::5005
nameserver 2a02:6b8::1:1
nameserver 2a02:6b8:0:3400::1
search yp-c.yandex.net yandex.net yandex.ru
EOF

# salt for some reason can't add this repos, but only in bootstrap script,
# so we adding them manually to prevent salt from executing states with this repos
apt-key adv --batch --keyserver keyserver.ubuntu.com --recv-keys 68576280
bash -c "echo \"deb https://deb.nodesource.com/node_10.x bionic main\" > /etc/apt/sources.list.d/nodesource.list"

apt-key adv --batch --keyserver keyserver.ubuntu.com --recv-keys E298A3A825C0D65DFD57CBB651716619E084DAB9
bash -c "echo \"deb https://cloud.r-project.org/bin/linux/ubuntu bionic-cran35/\" > /etc/apt/sources.list.d/r-project.list"

apt-key adv --batch --keyserver keyserver.ubuntu.com --recv-keys E0C56BD4
bash -c "echo \"deb https://repo.yandex.ru/clickhouse/deb/stable/ main/\" > /etc/apt/sources.list.d/clickhouse.list"

# install dependencies for salt
apt-get -y -qq update
apt-get -y -qq install python-apt python-dateutil python-jinja2 python-msgpack python-requests python-concurrent.futures python-tornado python-yaml python-systemd python-psutil python-gnupg python-crypto python-zmq dctrl-tools

# install salt 2019.2.0
wget -q https://proxy.sandbox.yandex-team.ru/1737116775 -O /srv/salt.tar
tar -xf /srv/salt.tar -C /srv
dpkg --install /srv/salt-common_2019.2.0+ds-1_all.deb /srv/salt-minion_2019.2.0+ds-1_all.deb
rm /srv/salt.tar /srv/salt-common_2019.2.0+ds-1_all.deb /srv/salt-minion_2019.2.0+ds-1_all.deb

tee <<EOF /etc/salt/minion
file_client: local
log_level: DEBUG
random_master: True

file_roots:
  base:
  - /srv/states
  - /srv/extensions

pillar_roots:
  base:
  - /srv/pillar
EOF

service salt-minion restart
sudo salt-call -l debug --local state.apply
service salt-minion stop

rm -rf /srv/states /srv/pillar /srv/extensions

tee <<EOF /etc/salt/minion
---
master: $master_host
ipv6: True
log_level: DEBUG
rejected_retry: True
master_alive_interval: 60
random_master: True
EOF

# By default minion_id contains hostname where package was installed (in our case - sandbox).
# When starts, it will automaticly gets current hostname
rm /etc/salt/minion_id

systemctl unmask apt-daily.service
systemctl unmask apt-daily-upgrade.service
systemctl enable apt-daily.service
systemctl enable apt-daily-upgrade.service
