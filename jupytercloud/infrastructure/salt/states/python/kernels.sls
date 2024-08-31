include:
- jupyterhub.service
- .essentials

{% for ver in ('2.7', '3.7') %}

{% set pyrsistent = 'pyrsistent' %}
{% set decorator = 'decorator' %}
{% set terminado = 'terminado' %}
{% set regex = 'regex' %}

{% if ver == '2.7' %}
  {% set pyrsistent = 'pyrsistent==0.16.1' %}
  {% set decorator = 'decorator==4.4.2' %}
  {% set terminado = 'terminado==0.8.3' %}
  {% set regex = 'regex==2021.11.10' %}
{% endif %}

{% set ipykernel_deps = [terminado, 'ipykernel', 'ipython', 'ipywidgets', 'pyzmq', 'tornado', pyrsistent, decorator, regex] %}
{% set pip_bin = "/usr/local/bin/pip" + ver %}

raw-ipykernel{{ver}}-deps:
  pip.installed:
  - pkgs: {{ ipykernel_deps }}
  - upgrade: True
  - ignore_installed: True
  - bin_env: {{pip_bin}}
  - require:
    - pip: pip{{ver}}-latest
  - unless:
    {% for package in ipykernel_deps %}
    - "/usr/local/bin/pip{{ver}} show {{package}} | fgrep /usr/local/lib/python{{ver}}/dist-packages"
    {% endfor %}

ipykernel{{ver}}-deps:
  pip.installed:
  - pkgs: {{ ipykernel_deps }}
  - upgrade: True
  - bin_env: {{pip_bin}}
  - require:
    - pip: pip{{ver}}-latest
    - pip: raw-ipykernel{{ver}}-deps

{% endfor %}

python2-kernel:
  file.managed:
  - name: '/usr/local/share/jupyter/kernels/python2/kernel.json'
  - source: 'salt://python/kernel_python2.json'
  - makedirs: True
  - mode: 755
  - require:
    - pip: ipykernel2.7-deps

python3-kernel:
  file.managed:
  - name: '/usr/local/share/jupyter/kernels/python3/kernel.json'
  - source: 'salt://python/kernel_python3.json'
  - makedirs: True
  - mode: 755
  - require:
    - pip: ipykernel3.7-deps
