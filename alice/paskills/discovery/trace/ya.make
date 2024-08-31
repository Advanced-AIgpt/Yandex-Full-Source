OWNER(g:paskills)

PY3_PROGRAM()

PY_SRCS(
    MAIN trace.py
    saas/__init__.py
    saas/ask.py
    saas/cfg.py
    saas/dialog.py
    wizard/__init__.py
    wizard/ask.py
    wizard/cfg.py
)

PEERDIR(
    contrib/python/requests
)

END()
