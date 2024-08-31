PY23_LIBRARY()


OWNER(zubchick)

PEERDIR(
    alice/review_bot/ybot/core

    contrib/python/attrs
    contrib/python/crontab
    contrib/python/gevent
    contrib/python/python-telegram-bot
    contrib/python/pytz
)

PY_SRCS(
    __init__.py
    backdoor.py
    cron.py
    ping.py
    telegram.py
)

END()
