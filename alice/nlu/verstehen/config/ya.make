PY2_LIBRARY()

OWNER(
    artemkorenev
    g:alice_quality
)

PY_SRCS(
    NAMESPACE verstehen.config
    __init__.py
    data_config.py
    generic_config.py
    metrics_config.py
    util.py
)

END()
