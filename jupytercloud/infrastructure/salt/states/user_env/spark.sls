include:
- python
- .java

spark-package-remove:
  pkg.removed:
  - pkgs:
    - spark-yt-client

{% for ver in ('2.7', '3.7') %}
{% set pip_bin = "/usr/local/bin/pip" + ver %}

python{{ver}}-spyt-packages:
  pip.installed:
  - pkgs:
    - yandex-spyt>=1.0.1,<2.0.0
  - upgrade: True
  - bin_env: {{pip_bin}}
  - index_url: https://pypi.yandex-team.ru/simple
  - require:
    - pip: python{{ver}}-pip-essentials
    - pkg: java-package

{% endfor %}
