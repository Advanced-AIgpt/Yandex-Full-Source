include:
- salt-minion

render-packages:
  pkg.latest:
  - pkgs:
    - graphviz
    - ghostscript
    - imagemagick
    - libcairo2-dev
    - libmagickcore-dev
    - netpbm
    - pandoc
    - xml-core
    - texlive-xetex
    - texlive-fonts-recommended
    - texlive-generic-recommended

misc-analytics-deps:
  pkg.latest:
  - pkgs:
    - gfortran
    - libatlas-base-dev
    - libblas-dev
    - liblapack-dev
    - libprotobuf-dev
    - libprotoc-dev
    - libthrift-dev
    - llvm-6.0-dev
    - protobuf-compiler
    - thrift-compiler
    - libgeos-dev
    - libsasl2-dev
    - libsnappy-dev
    - libspatialindex-dev
    - yandex-advq7-grep-python
    - libgdal-dev
    - libgdal20

database-packages:
  pkg.latest:
  - pkgs:
    - freetds-dev
    - tdsodbc
    - libpq-dev
    - libpq5
    - libmysqlclient-dev
    - mysql-client
    - postgresql-client
    - sqlite3
    - unixodbc-dev
