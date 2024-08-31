PY3_PROGRAM(alice_battle_angel_bot)

OWNER(sparkle)

PEERDIR(
    contrib/python/python-telegram-bot
    ydb/public/sdk/python
    library/python/flask
    sandbox/common/rest
)

PY_SRCS(
    __main__.py
    branch_parser.py
    common.py
    digest_parser.py
    evo_fails_parser.py
    ydb_queue.py
)

END()
