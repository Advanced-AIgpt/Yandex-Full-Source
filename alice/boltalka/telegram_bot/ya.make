PY3_PROGRAM()

OWNER(
    nzinov
    g:alice_boltalka
)

PY_SRCS(
    __main__.py
)

PEERDIR(
    contrib/python/python-telegram-bot
    alice/boltalka/telegram_bot/lib
)

NO_CHECK_IMPORTS()

END()
