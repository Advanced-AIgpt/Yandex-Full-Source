OWNER(
    g:paskills
)

PY3_LIBRARY()

PY_SRCS(
    __init__.py
    projects.py
)

PEERDIR(
    library/python/monitoring/solo/objects/solomon/v2
)

END()
