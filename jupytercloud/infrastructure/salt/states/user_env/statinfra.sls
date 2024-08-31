include:
- python

statbox-repo-all:
  pkgrepo.managed:
  - name: "deb http://dist.yandex.ru/statbox-common stable/all/"
  - file: /etc/apt/sources.list.d/statbox.list

statbox-repo-amd64:
  pkgrepo.managed:
  - name: "deb http://dist.yandex.ru/statbox-common stable/amd64/"
  - file: /etc/apt/sources.list.d/statbox.list

statbox-common-config:
  pkg.latest:
  - require:
    - pkgrepo: statbox-repo-all
    - pkgrepo: statbox-repo-amd64

python2-statinfra-packages:
  pip.installed:
  - pkgs:
    - python-statinfra
    - jsonschema==3.2.0
  - upgrade: True
  - bin_env: /usr/local/bin/pip2.7
  - index_url: https://pypi.yandex-team.ru/simple
  - require:
    - pip: python2.7-pip-essentials

statinfra-environment-spec:
  file.managed:
  - name: /usr/share/statbox-class-filestorage/Environment/spec.yaml
  - makedirs: True
  - contents: 'mrparser'
