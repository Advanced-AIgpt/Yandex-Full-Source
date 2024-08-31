include:
- python
- .deb_env

{% for ver in ('2.7', '3.7') %}

{% set distutils_bad_packages = ['simplejson', 'PyYAML'] %}
{% set pip_bin = "/usr/local/bin/pip" + ver %}

python{{ver}}-distutils-installed-hack:
  pip.installed:
  - pkgs: {{distutils_bad_packages}}
  - upgrade: True
  - ignore_installed: True
  - bin_env: {{pip_bin}}
  - require:
    - pip: python{{ver}}-pip-essentials
  - unless:
    {% for package in distutils_bad_packages %}
    - "{{pip_bin}} show {{package}} | fgrep /usr/local/lib/python{{ver}}/dist-packages"
    {% endfor %}

python{{ver}}-misc-packages:
  pip.installed:
  - pkgs:
    - PyYAML
    - YaDiskClient
    - appdirs
    - argcomplete
    - beautifulsoup4
    - certifi
    - easywebdav
    - graphviz
    - ipdb
    - m2crypto
    - python-dateutil
    - requests
    - simplejson
    - tqdm
    - ujson
    - yandex.translate
    - xlsxwriter
    - contextlib2
    - openpyxl
    - xlrd
    - httplib2
    - mysqlclient
    - psycopg2
    - sqlalchemy
    - clickhouse-driver
    - clickhouse-sqlalchemy
    {% if ver == '3.7' %}
    - pytz
    - ipython-sql
    - unidecode
    {% endif %}
  - upgrade: True
  - bin_env: {{pip_bin}}
  - require:
    - pkg: misc-analytics-deps
    - pip: python{{ver}}-pip-essentials
    - pip: python{{ver}}-distutils-installed-hack

python{{ver}}-analytilcs-packages:
  pip.installed:
  - pkgs:
    - PySAL
    - catboost
    - cufflinks
    - gensim
    - matplotlib
    - nltk
    - numpy
    - pandas
    - plotly
    - pymorphy2
    - scipy
    - seaborn
    - shapely
    - geohash
    - geopandas
    - folium
    {% if ver == '3.7' %}
    - statsmodels
    - scikit-learn
    - sklearn-pandas
    - xgboost==1.5.2  # fixed version due to 404 on pypi.yandex-team.ru for 1.6.0
    {% else %}
    - Fiona==1.8.18
    - xgboost==0.82
    {% endif %}
  - upgrade: True
  - bin_env: {{pip_bin}}
  - require:
    - pkg: misc-analytics-deps
    - pip: python{{ver}}-pip-essentials

python{{ver}}-heavy-analytilcs-packages:
  # this packages requires too much MEM for upgrading,
  # so we installing last version in our QEMU image,
  # but no futher updates
  pip.installed:
  - pkgs:
    - Pillow
    - tensorflow
    - markdown<=3.2.1
  - upgrade: False
  - bin_env: {{pip_bin}}
  - require:
    - pkg: misc-analytics-deps
    - pip: python{{ver}}-pip-essentials

{% endfor %}

python3.7-pystan:
  pip.installed:
  - pkgs:
    - httpstan==4.6.1
    - pystan==3.3.0  # upgrade if you wish
  - upgrade: True
  - bin_env: /usr/local/bin/pip3.7
  - require:
    - pip: python3.7-analytilcs-packages
