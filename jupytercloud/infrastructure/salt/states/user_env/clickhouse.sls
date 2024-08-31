include:
- .generic

clickhouse-repo:
  pkgrepo.managed:
  - name: "deb https://repo.yandex.ru/clickhouse/deb/stable/ main/"
  - keyserver: keyserver.ubuntu.com
  - keyid: E0C56BD4
  - file: /etc/apt/sources.list.d/clickhouse.list
  - clean_file: True
  - require:
    - pkg: misc-devops-packages

clickhouse-client-package:
  pkg.latest:
  - pkgs:
    - clickhouse-client
