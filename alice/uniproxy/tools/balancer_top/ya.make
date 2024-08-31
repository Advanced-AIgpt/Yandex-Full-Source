PY3_PROGRAM(top_requests)
OWNER(g:voicetech-infra)

PY_SRCS(
    MAIN __main__.py
    __init__.py
    collect_logs.py
    nanny.py
    top.py
)

END()
