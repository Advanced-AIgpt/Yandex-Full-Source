PY2_PROGRAM()

OWNER(olegtitov)

PEERDIR(
    alice/vins/api
    alice/vins/api_helper
    contrib/python/gunicorn
    alice/vins/apps/crm_bot
)

PY_SRCS(
    __main__.py
)

END()
