#!/usr/bin/env bash
set -euxo pipefail

# yandex internal stuff + devops tools + production utils
apt-get update
apt-get --yes upgrade
apt-get --yes install \
    python3.9 python3.9-distutils \
    yandex-internal-root-ca yandex-arc-launcher \
    \
    neovim ncdu nano htop atop iotop jq less tmux curl \
    psmisc wget rsync bash-completion console-setup \
    \
    cron openssh-server logrotate zstd rsyslog

ln -sf /usr/bin/python3.9 /opt/venv/bin/python3.9
pip3.9 install --no-cache-dir --upgrade -i https://pypi.yandex-team.ru/simple \
    supervisor supervisord-dependent-startup yandex-passport-vault-client

wget "https://crls.yandex.net/allCAs.pem" -qO /srv/allCAs.pem

mv /etc/cron.daily/logrotate /etc/cron.hourly/
mkdir /etc/supervisor.d /var/log/supervisor /srv/yav-deploy /var/cache/unified_agent

# fix old Debian bug and enable cron
sed -i '/session    required     pam_loginuid.so/c\#session    required   pam_loginuid.so' /etc/pam.d/cron
sed -i 's/#cron/cron/' /etc/rsyslog.conf

# for interactive use
update-alternatives --install /usr/bin/vim vim /usr/bin/nvim 80
tee -a /root/.bashrc << 'EOF'

PATH="/opt/venv/bin:$PATH"
JUPYTER_ENV="${DEPLOY_STAGE_ID#*jupytercloud-hub-}"
PS1='${debian_chroot:+($debian_chroot)}\u@${DEPLOY_UNIT_ID}-${DEPLOY_NODE_DC}.${JUPYTER_ENV}:\w\$ '

eval "$(dircolors -b)"
ls_params='--color=auto -v'
alias ls="command ls ${ls_params}"
alias la="command ls -AlF ${ls_params}"
alias ll="command ls -alF ${ls_params}"
alias lh="command ls -hlF ${ls_params}"
alias l="command ls -CF ${ls_params}"

source /root/.scripts.sh
EOF
