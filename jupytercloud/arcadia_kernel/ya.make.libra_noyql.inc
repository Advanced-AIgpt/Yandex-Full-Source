# Файл инклюдится в py2/py3 ядрах

PEERDIR(
    jupytercloud/nirvana
)

PEERDIR(
    # jupyter-specific
    contrib/python/ipdb
    contrib/python/ipywidgets

    # general purpose
    contrib/python/requests
    contrib/python/dateutil
    contrib/python/pytz
    contrib/python/M2Crypto
    contrib/python/PyYAML
    contrib/python/tqdm
    contrib/python/simplejson
    contrib/python/beautifulsoup4
    contrib/python/easywebdav
    contrib/python/xlrd
    contrib/python/httplib2
    contrib/python/selenium
    contrib/python/openpyxl

    # DB-related
    contrib/python/psycopg2
    contrib/python/ipython-sql
    contrib/python/clickhouse-driver
    contrib/python/clickhouse-sqlalchemy
    contrib/python/kazoo

    # analytics stack
    contrib/python/pandas
    contrib/python/numpy
    contrib/python/matplotlib
    contrib/python/scipy
    contrib/python/plotly
    contrib/python/chart-studio
    contrib/python/Shapely
    contrib/python/python-geohash
    contrib/python/seaborn
    contrib/python/scikit-learn
    contrib/python/statsmodels
    contrib/python/nltk
    contrib/python/pymorphy2
    contrib/python/fbprophet
    contrib/python/gensim
    contrib/python/pyresample
    contrib/python/sympy
    contrib/python/python-pptx

    # yandex-specific
    library/python/startrek_python_client
    library/python/statface_client
    library/python/vault_client
    library/python/nirvana
    library/python/nirvana_api
    library/python/blackbox
    sandbox/common
    jupytercloud/library
    bindings/python/easyparser_lib

    # devops stack
    infra/yasm/yasmapi
    # XXX: both modules have infra/nanny/nanny_proto_schemas/proto dep
    # which conflicts with libra dep by protobuf extension numbers
    # infra/nanny/nanny_services_rest
    # saas/tools/devops/lib23

    # python/qb2-stack
    yql/library/python
    statbox/qb2_core
    statbox/qb2
    statbox/bindings/signurl
    statbox/libstatbox/python  # statbox_bindings2
    geobase/python/static
    quality/user_sessions/libra_arc/noyql
    quality/logs/baobab/api/python_arc/static
    statbox/nile
    yt/yt/python/yt_yson_bindings
    metrika/uatraits/python
    library/python/auth_client_parser
)

IF(PYTHON2)
PEERDIR(
    contrib/python/sqlalchemy/sqlalchemy-1.3
    library/python/clickhouse_client
    statbox/python-statinfra
    advq/grep/pylib
    quality/user_sessions/py_amon
    comdep_analytics/exceller_py2
)
ENDIF()

IF(PYTHON3)
PEERDIR(
    contrib/python/sqlalchemy/sqlalchemy-1.4
    contrib/python/googleads
    contrib/python/facebook-business
    contrib/python/Office365-REST-Python-Client
    contrib/python/langcodes
    contrib/python/language-data

    library/python/toloka-kit
    april/lib/py/exceller
)
ENDIF()
