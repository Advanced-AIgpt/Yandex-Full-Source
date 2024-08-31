# Тут происходит дичь.
# 1) Мы скачиваем get-pip.py
# 2) Чтобы иметь возможность его выполнить, нам нужен pip (на самом деле distutils)
# 3) Мы ставим deb-пакет с pip, но только если соответствующего бинарника еще нет
# 4) Мы вополняем get-pip.py, он ставит pip в /usr/local, но только если там его еще нет
# 5) Мы сносим deb-пакет c pip (или просто убеждаемся, что его нет)
# 6) Мы апгрейдим pip через /usr/local/bin/pip (на самом деле, там и так последний стоит,
#    но оно зафейлится, если что-то будет не так)

include:
- python.deb

deb-pip-removed-2.7:
  pkg.removed:
  - name: python-pip

deb-pip-removed-3.7:
  pkg.removed:
  - name: python3-pip

pip.conf:
  file.managed:
    - name: /etc/pip.conf
    - source: salt://python/pip.conf
    - user: root
    - group: root
    - mode: 644

python-distutils:
  pkg.latest:
  - pkgs:
    - python-distutils-extra
    - python3-distutils-extra

{% set versions = [
    ('2.7', 'https://bootstrap.pypa.io/pip/2.7/get-pip.py', '20.3.4'),
    ('3.7', 'https://bootstrap.pypa.io/get-pip.py', '21.3.1')
] %}

{% for ver, url, pip_ver in versions %}

get-pip-{{ver}}.py:
  cmd.run:
  - name: "curl {{url}} -o /srv/get-pip-{{ver}}.py"
  - unless:
    - "ls /srv/get-pip-{{ver}}.py"

pip{{ver}}-manual:
  cmd.run:
  - name: "python{{ver}} /srv/get-pip-{{ver}}.py"
  - unless:
    - ls /usr/local/bin/pip{{ver}}
  - require:
    - pkg: python{{ver}}
    - pkg: python-distutils
    - cmd: get-pip-{{ver}}.py
    - file: pip.conf

pip{{ver}}-latest:
  pip.installed:
  - name: pip=={{pip_ver}}
  - bin_env: /usr/local/bin/pip{{ver}}
  - upgrade: True
  - require:
    - cmd: pip{{ver}}-manual
    - pkg: deb-pip-removed-{{ver}}

pip{{ver}}-alternative:
  alternatives.install:
  - name: pip
  - link: /usr/local/bin/pip
  - path: /usr/local/bin/pip{{ver}}
  - priority: {{ 10 * loop.index }}
  - require:
    - cmd: pip{{ver}}-manual

{% endfor %}

pip-auto-alternatives:
  alternatives.auto:
  - name: pip
  - require:
    - alternatives: pip2.7-alternative
    - alternatives: pip3.7-alternative
