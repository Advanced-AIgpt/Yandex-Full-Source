PY3_PROGRAM()

OWNER(
    artemkorenev
    g:alice_boltalka
)

PY_SRCS(
    __init__.py
    __main__.py
    telegram_bot.py
)

PEERDIR(
    alice/boltalka/generative/tfnn/infer_lib
    alice/boltalka/generative/tfnn/preprocess
    alice/boltalka/generative/training/data/tokenizer_py

    # 3rd party
    contrib/python/Flask-RESTful
    contrib/python/python-telegram-bot
    contrib/python/requests
)

END()
