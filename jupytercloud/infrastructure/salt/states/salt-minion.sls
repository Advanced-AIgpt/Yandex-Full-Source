saltstack-repo-absent:
  file.absent:
  - name: '/etc/apt/sources.list.d/saltstack.list'

r-key:
  module.run:
  - name: pkg.add_repo_key
  - keyid: E298A3A825C0D65DFD57CBB651716619E084DAB9
  - keyserver: keyserver.ubuntu.com

r-repo:
  pkgrepo.managed:
  - humanname: R-project repo
  - name: "deb https://cloud.r-project.org/bin/linux/ubuntu bionic-cran35/"
  - file: /etc/apt/sources.list.d/r-project.list
  - keyid: E298A3A825C0D65DFD57CBB651716619E084DAB9
  - keyserver: keyserver.ubuntu.com
  - require:
    - module: r-key

minion-runtime-pip:
  pkg.installed:
  - pkgs:
    - python-pip
  - unless:
    - pip2  # it may be installed via pip itself

minion-runtime:
  pkg.installed:
  - pkgs:
    - apt-utils

# it requires configuration with master
{% if grains.get('jupyter_cloud_user') %}
minion-ping-event:
   schedule.present:
   - function: event.fire_master
   - job_args: ["", ping]
   - seconds: 120
   - splay: 5
   - return_job: False
{% endif %}

sandbox-client:
  pip.installed:
  - name: sandbox-library
  - upgrade: True
  - index_url: https://pypi.yandex-team.ru/simple
  - reload_modules: True
  - force_reload_modules: True
  - require:
    - file: old-client-absense

old-client-absense:
  file.absent:
  # right path is /usr/local/lib/python2.7/dist-packages/sandbox
  - name: /usr/lib/python2.7/dist-packages/sandbox
  - reload_modules: True
  - force_reload_modules: True

apt-daily-service-disabled:
  service.masked:
  - name: apt-daily

apt-daily-service-dead:
  service.dead:
  - name: apt-daily
  - enable: False

apt-daily-upgrade-service-disabled:
  service.masked:
  - name: apt-daily-upgrade

apt-daily-upgrade-service-dead:
  service.dead:
  - name: apt-daily-upgrade
  - enable: False

apt-cron-disabled:
  file.absent:
  - name: /etc/cron.daily/apt-compat

apt-daily-killed:
  test.nop:
  - require:
    - service: apt-daily-service-disabled
    - service: apt-daily-service-dead
    - service: apt-daily-upgrade-service-disabled
    - service: apt-daily-upgrade-service-dead
    - file: apt-cron-disabled

# BUILD_PORTO_LAYER for some reason is keeping secret_env file
# after building the image
/secret_env:
  file.absent: []
