include:
- .service
- node
- node.config

{% set versions = [
    ('hub-1.1', pillar['jupyter-dir-1.1'], '1.1.0')
] %}


jupyterlab_extensions_cleaner:
  file.managed:
  - name: /srv/clean_jupyterlab_ext.py
  - source: salt://jupyterhub/clean_jupyterlab_ext.py
  - makedirs: True
  - user: root
  - group: root
  - mode: 500

{% for hub_version, venv_dir, pip_version in versions %}

jupyterlab-{{ hub_version }}:
  pip.installed:
  - pkgs:
    - 'jupyterlab==2.1'
    - 'jupyterlab-server==1.2.0'
  - bin_env: {{ venv_dir }}/bin/pip
  - user: root
  - require:
    - pip: jupyterhub-{{ hub_version }}
    - cmd: enable-jupyterlab-swap
# Стремно, перегагрузит людям все
# - watch_in:
#   - supervisord: jupyterhub-service

{% if grains.get('jupyter_cloud_user') %}

jupyterlab-extensionmanager-extension-{{ hub_version }}:
  file.managed:
  - name: "/home/{{grains['jupyter_cloud_user']}}/.jupyter/lab/user-settings/@jupyterlab/extensionmanager-extension/plugin.jupyterlab-settings"
  - makedirs: True
  - user: {{grains['jupyter_cloud_user']}}
  - mode: 644
  - contents: |
      {
        "enabled": true,
        "disclaimed": false
      }

jupyterlab_set_user-{{ hub_version }}:
  cmd.run:
  - name: "chown -R {{grains['jupyter_cloud_user']}} {{venv_dir}}/share/jupyter/lab"
  - require:
    - cmd: build_jupyterlab_extensions-{{ hub_version }}
  - hide_output: True

{% endif %}

{% set extensions = [
  ('@jupyter-widgets/jupyterlab-manager@2', '@jupyter-widgets/jupyterlab-manager v2'),
  ('@pyviz/jupyterlab_pyviz@1.0.3', '@pyviz/jupyterlab_pyviz v1.0.3'),
  ('@bokeh/jupyter_bokeh@2.0.2', '@bokeh/jupyter_bokeh v2.0.2'),
  ('jupyterlab-plotly@4.7.1', 'jupyterlab-plotly v4.7.1'),
] %}

{% for extension, check_string in extensions %}

jupyterlab_extension_{{extension}}-{{ hub_version }}:
  cmd.run:
  - name: {{venv_dir}}/bin/jupyter labextension install {{extension}} --no-build 
  - env:
    - NODE_OPTIONS: "--max_old_space_size=768"
  - hide_output: True
  - unless: "{{venv_dir}}/bin/jupyter labextension list 2>&1 | fgrep '{{check_string}}' | fgrep OK"
  - require:
    - pip: jupyterlab-{{ hub_version }}
  - retry:
    attempts: 3
    until: True
    interval: 60
    splay: 10

{% endfor %}

{% set pip_extensions = [
    'jupyterlab-infra-buzzer',
    'jupyterlab-yndxbug',
    'jupytercloud-lab-metrika',
    'jupyterlab-arcadia-share',
    'jupytercloud-lab-nirvana',
    'jupytercloud-lab-lib-extension',
    'jupytercloud-lab-vault',
] %}

jupyterlab_pip_extensions-{{ hub_version }}:
  pip.installed:
  - pkgs: {{ pip_extensions }}
  - upgrade: True
  - bin_env: {{ venv_dir }}/bin/pip
  - index_url: https://pypi.yandex-team.ru/simple
  - require:
    - pip: jupyterlab-{{ hub_version }}

jupyterlab_clear_extensions-{{ hub_version }}:
  cmd.run:
  - name: "/srv/clean_jupyterlab_ext.py --venv-dir {{venv_dir}} --extensions {{ pip_extensions | join(' ') }}"
  - require:
    - file: /srv/clean_jupyterlab_ext.py

build_jupyterlab_extensions-{{ hub_version }}:
  cmd.run:
  - name: "{{venv_dir}}/bin/jupyter lab build --minimize=False --dev-build=False"
  - env:
    - NODE_OPTIONS: "--max_old_space_size=768"
  - require:
    - pip: jupyterlab-{{ hub_version }}
    - pip: jupyterlab_pip_extensions-{{ hub_version }}
    - cmd: jupyterlab_clear_extensions-{{ hub_version }}
    - pkg: nodejs
    - file: /usr/etc/npmrc
    {% for extension, _ in extensions %}
    - cmd: jupyterlab_extension_{{extension}}-{{ hub_version }}
    {% endfor %}
  - onlyif:
    - {{venv_dir}}/bin/jupyter labextension list 2>&1 | grep 'Build recommended'

disable-jupyterlab-extensions:
  file.managed:
  - name: "{{venv_dir}}/share/jupyter/lab/settings/page_config.json"
  - makedirs: True
  - mode: 644
  - contents: |
      {
          "disabledExtensions": [
          ]
      }

{% endfor %}

{% set swap_file = '/jupyterlab-swapfile' -%}

enable-jupyterlab-swap:
  cmd.run:
  {% if grains.get('jupyter_cloud_user') %}
  - name: |
      [ -f {{ swap_file }} ] || fallocate -l 1G {{ swap_file }}
      chmod 0600 {{ swap_file }}
      mkswap {{ swap_file }}
      swapon {{ swap_file }}
  - unless:
    - file {{ swap_file }} 2>&1 | grep -q "Linux/i386 swap"
  {% else %}
  - name: echo "enable-jupyterlab-swap stub"
  {% endif %}

disable-jupyterlab-swap:
  cmd.run:
  {% if grains.get('jupyter_cloud_user') %}
  - name: |
      swapoff {{ swap_file }}
      rm {{ swap_file }}
  - onlyif:
    - file {{ swap_file }} 2>&1 | grep -q "Linux/i386 swap"
  {% else %}
  - name: echo "disable-jupyterlab-swap stub"
  {% endif %}
  - require:
    {% for hub_version, venv_dir, pip_version in versions %}
    - cmd: build_jupyterlab_extensions-{{ hub_version }}
    {% endfor %}
