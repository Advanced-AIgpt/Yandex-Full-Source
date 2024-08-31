include:
- .service

{% set versions = [
    ('hub-1.1', pillar['jupyter-dir-1.1'], '1.1.0')
] %}

{% for hub_version, venv_dir, pip_version in versions %}

{% for package in pillar['nbextensions_packages'] %}

{{package}}-{{hub_version}}:
  pip.installed:
  - pkgs:
    - {{package}}
  - bin_env: {{venv_dir}}/bin/pip
  - user: root
  - require:
    - pip: jupyterhub-{{hub_version}}

{% endfor %}

{% for package, command in pillar['nbextensions_activate_commands'].items() %}

{{package}}_enable-{{hub_version}}:
  cmd.run:
  - name: {{venv_dir}}/bin/{{command}}
  - runas: root
  - onchanges:
    - pip: {{package}}-{{hub_version}}

{% endfor %}

# enable default extensions
{% for extension, package in pillar['nbextensions_to_enable'] %}

jupyter_nbextension_{{extension}}_enable-{{hub_version}}:
  cmd.run:
  - name: "{{venv_dir}}/bin/jupyter nbextension enable {{extension}} --sys-prefix"
  - runas: root
  - require:
    - cmd: jupyter_contrib_nbextensions_enable-{{hub_version}}
    - pip: {{package}}-{{hub_version}}
  - unless:
    - "{{venv_dir}}/bin/jupyter nbextension list 2>&1 2>&1 | fgrep {{extension}} | fgrep enable"

{% endfor %}

jupytercloud_extensions-{{hub_version}}:
  pip.installed:
  - pkgs:
    - jupyter-infra-buzzer
    - jupyter-arcadia-share
    - jupyter-yndxbug
    - jupyter-metrika
  - bin_env: {{venv_dir}}/bin/pip
  - user: root
  - upgrade: True
  - index_url: https://pypi.yandex-team.ru/simple
  - require:
    - pip: jupyterhub-{{hub_version}}

{% endfor %}
