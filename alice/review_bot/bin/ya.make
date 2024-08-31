PY3_PROGRAM(review_bot)

OWNER(zubchick)

PEERDIR(
    contrib/python/click
    alice/review_bot/lib
    alice/review_bot/ybot/core
    alice/review_bot/ybot/modules
)

PY_SRCS(
    MAIN main.py
)

END()
