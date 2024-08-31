{% set root = pillar['pyenv-root'] %}

pyenv-deps:
  pkg.installed:
  - pkgs:
    - make
    - build-essential
    - libssl-dev
    - zlib1g-dev
    - libbz2-dev
    - libreadline-dev
    - libsqlite3-dev
    - wget
    - curl
    - llvm
    - git

{% for python in pillar['python-versions'] %}

{{ python }}:
  pyenv.installed:
  - require:
    - pkg: pyenv-deps

{% endfor %}

{% for short, long in pillar['python-versions-symlinks'].items() %}

python{{ short }}:
  file.symlink:
  - name: {{ root }}/versions/{{ short }}
  - target: {{ root }}/versions/{{ long }}
  - require:
    - pyenv: python-{{ long }}

{% endfor %}

# to do this truly workable we need do edit /etc/sudoers to keep
# PYENV_ROOT and PATH
# at this point we desided to use system python for user environment
# python3-global:
#   cmd.run:
#   - name: "{{ root }}/bin/pyenv global 3"
#   - runas: root
#   - env:
#     - PYENV_ROOT: {{ root }}
#   - require:
#     - file: python3
# 
# pyenv_init:
#   file.append:
#   - name: /etc/bash.bashrc
#   - text:
#     - "export PYENV_ROOT={{ root }}"
#     - 'export PATH="$PYENV_ROOT/bin:$PATH"'
#     - 'eval "$(pyenv init -)"'
#   - require:
#     - cmd: python3-global
# 
# pyenv_shims:
#    file.directory:
#    - name: {{ root }}/shims
#    - user: root
#    - group: root
#    - dir_mode: 777
#    - recurse:
#      - user
#      - group
#      - mode
#    - require:
#    {% for python in pillar['python-versions'] %}
#      - pyenv: {{ python }}
#    {% endfor %}
