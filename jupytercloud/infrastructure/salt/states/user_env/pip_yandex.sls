include:
- python
- .deb_env

{% for ver in ('2.7', '3.7') %}

python{{ver}}-yandex-packages:
  pip.installed:
  - pkgs:
    - nile
    - qb2-core
    - statbox_bindings2
    - yql
    - yandex-yt
    - sandbox-library
    - blackbox
    - yandex-yt-yson-bindings
    - business_models
    - metrikatraficsource-bindings
    - qb2
    - signurl-bindings
    - python-statface-client
    - yandex-passport-vault-client
    - startrek_client
    - yandex-type-info
  - upgrade: True
  - bin_env: /usr/local/bin/pip{{ver}}
  - index_url: https://pypi.yandex-team.ru/simple
  - require:
    - pkg: misc-analytics-deps
    - pip: python{{ver}}-pip-essentials

python{{ver}}-jupytercloud-packages:
  pip.installed:
  - pkgs:
    - jupytercloud
  - upgrade: True
  - bin_env: /usr/local/bin/pip{{ver}}
  - index_url: https://pypi.yandex-team.ru/simple
  - require:
    - pkg: misc-analytics-deps
    - pip: python{{ver}}-pip-essentials

{% endfor %}
