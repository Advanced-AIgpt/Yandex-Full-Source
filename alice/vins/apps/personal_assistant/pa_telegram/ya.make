PY2_PROGRAM()

OWNER(g:alice)

PEERDIR(
    alice/vins/apps/personal_assistant
)

PY_SRCS(
    alice/vins/apps/personal_assistant/personal_assistant/pa_bot.py=__main__
)

END()
