include:
- .pip

python-deb-essentials:
  pkg.latest:
  - pkgs:
    - python-dev
    - python3.7-dev
    - libpython3.7-dev
    - libssl-dev
    - libffi-dev
    - libyaml-0-2
    - libsqlite3-dev
    - libbz2-dev
    - libreadline-dev
    - zlib1g-dev
    - swig
    - libre2-dev

python2.7-pip-essentials:
  pip.installed:
  - pkgs:
    - wheel>=0.29.0,<0.32.0
    - setuptools<42.0.0
    - virtualenv
    - cython
  - upgrade: True
  - bin_env: /usr/local/bin/pip2.7
  - require:
    - pip: pip2.7-latest

python3.7-pip-essentials:
  pip.installed:
  - pkgs:
    - wheel
    - setuptools
    - virtualenv
    - cython
    - psutil
  - upgrade: True
  - bin_env: /usr/local/bin/pip3.7
  - require:
    - pip: pip3.7-latest
