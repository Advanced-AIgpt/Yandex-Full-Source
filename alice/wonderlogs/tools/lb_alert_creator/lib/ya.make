PY3_LIBRARY()

OWNER(g:wonderlogs)

PY_SRCS(
    alerts.py
    config.py
    entities.py
)

END()

RECURSE_FOR_TESTS(ut)
