PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:voicetech-infra)

PEERDIR(
    library/python/sanitizers
)

PY_SRCS(
    __init__.py
    checks.py
    misc.py
    messages.py
    ws_utils.py
    process.py
    daemon.py
)

END()
