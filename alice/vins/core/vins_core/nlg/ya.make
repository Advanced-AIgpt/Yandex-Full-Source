PY2_LIBRARY(vins_core.nlg)

OWNER(g:alice)

PEERDIR(
    contrib/python/attrs
    contrib/python/dateutil
    contrib/python/emoji
    contrib/python/Jinja2
)

PY_SRCS(
    NAMESPACE vins_core.nlg
    __init__.py
    filters.py
    nlg_extension.py
    template_nlg.py
    tests.py
)

END()

RECURSE_FOR_TESTS(ut)
