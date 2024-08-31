include:
- .taxi_repos
- jupyterhub.jupyterlab
- jupyterhub.service

{% if 'settings' in pillar %}
  {% set settings = (pillar['settings'] or '') | load_json %}
{% else %}
  {% set settings = {} %}
{% endif %}

{% if 'yandex_taxi_atlas_dashboards' in settings and settings['yandex_taxi_atlas_dashboards'] %}

{% set venv_dir = pillar['jupyter-dir-1.1'] %}
{% set extension = '@jupyter-voila/jupyterlab-preview@1.1.0' %}
{% set check_string = '@jupyter-voila/jupyterlab-preview v1.1.0' %}

yandex-taxi-atlas-dashboards:
  pip.installed:
  - pkgs:
    - voila==0.2.4
  - bin_env: {{venv_dir}}/bin/pip
  - user: root
  - index_url: https://pypi.yandex-team.ru/simple
  - watch_in:
    - service: jupyterhub-systemd
  cmd.run:
  - name: {{venv_dir}}/bin/jupyter labextension install {{extension}} --no-build
  - hide_output: True
  - unless: "{{venv_dir}}/bin/jupyter labextension list 2>&1 | fgrep '{{check_string}}' | fgrep OK"
  - require_in:
    - cmd: build_jupyterlab_extensions-hub-1.1
  pkg.latest:
  - require:
    - pkgrepo: taxi-repo-common-stable
    - pkgrepo: taxi-repo-bionic-unstable

{% else %}
yandex-taxi-atlas-dashboards:
  pkg.removed: []
{% endif %}
