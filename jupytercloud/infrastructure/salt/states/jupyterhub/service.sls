include:
- pyenv
- supervisor

delete-old-jupyterhub:
  file.absent:
  - name: /usr/local/pyenv/versions/jupyterhub

delete-old-wrapper:
  file.absent:
  - name: /srv/jupyterhub-singleuser-wrapper.sh

delete-old-service:
  file.absent:
  - name: /etc/supervisor/conf.d/jupyterhub.conf

{% set versions = [
    ('hub-1.1', pillar['jupyter-dir-1.1'], '1.1.0')
] %}

{% for hub_version, venv_dir, pip_version in versions %}

jupyterhub-venv-{{ hub_version }}:
  virtualenv.managed:
  - name: {{ venv_dir }}
  - system_site_packages: False
  - venv_bin: /usr/local/pyenv/versions/3/bin/pyvenv
  - require:
    - file: python3

jupyterhub-{{ hub_version }}:
  pip.installed:
  - pkgs:
    - jupyterhub=={{ pip_version }}
    - notebook==5.7.8
    - nbformat>=5
  - bin_env: "{{ venv_dir }}/bin/pip"
  - user: root
  - require:
    - virtualenv: jupyterhub-venv-{{ hub_version }}

pyenv-kernels-absent-{{hub_version}}:
  file.absent:
  - name: "{{ venv_dir }}/share/jupyter/kernels"
  - require:
    - pip: jupyterhub-{{hub_version}}

{% endfor %}

{% if grains.get('jupyter_cloud_user') %}

jupyterhub-systemd-config:
  file.managed:
  - name: /etc/systemd/system/jupyterhub-user.service
  - source: salt://jupyterhub/jupyterhub-user.service
  - makedirs: True
  - user: root
  - group: root
  - mode: 600
  - template: jinja
  - context:
    jupyterhub_username: "{{ grains['jupyter_cloud_user'] }}"
    jupyterhub_env: {{ pillar['jupyterhub_env'].replace(',', ' ') }}
    jupyterhub_path: "{{ pillar['jupyter-dir-1.1'] }}/bin"
    {% if grains['jupyter_cloud_user'][0]|int(-1) == -1 %}
    jupyterhub_alias: "{{ grains['jupyter_cloud_user'] }}"
    jupyterhub_command: "{{ pillar['jupyter-dir-1.1'] }}/bin/jupyterhub-singleuser --ip ::"
    {% else %}
    jupyterhub_alias: "root"
    jupyterhub_command: "/usr/bin/sudo -EHu {{ grains['jupyter_cloud_user'] }} {{ pillar['jupyter-dir-1.1'] }}/bin/jupyterhub-singleuser --ip ::"
    {% endif %}
  - require:
    - pip: jupyterhub-hub-1.1

jupyterhub-systemd:
  service.running:
  - name: jupyterhub-user
  - enable: True
  - watch:
    - file: jupyterhub-systemd-config

{% else %}

jupyterhub-systemd-config:
  file.managed:
  - name: /etc/systemd/system/jupyterhub-user.service
  - source: salt://jupyterhub/jupyterhub-no-user.service
  - makedirs: True
  - user: root
  - group: root
  - mode: 600
  - template: jinja
  - context:
    jupyterhub_command: "{{ pillar['jupyter-dir-1.1'] }}/bin/jupyterhub-singleuser --ip ::"
  - require:
    - pip: jupyterhub-hub-1.1

jupyterhub-systemd:
  service.dead:
  - name: jupyterhub-user
  - enable: False

{% endif %}

jupyterhub-systemd-reload:
  module.run:
  - name: service.systemctl_reload
  - require:
    - file: jupyterhub-systemd-config

jupyterhub-logrotate:
  file.managed:
  - name: /etc/logrotate.d/jupyterhub
  - source: salt://jupyterhub/jupyterhub-logrotate.conf
  - mode: 644
