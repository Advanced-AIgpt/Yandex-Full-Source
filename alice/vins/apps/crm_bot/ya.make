PY2_LIBRARY()

OWNER(
    olegtitov
)

PEERDIR(
    alice/vins/apps/personal_assistant
)

INCLUDE(${ARCADIA_ROOT}/alice/vins/apps/crm_bot/resources.inc)

PY_SRCS(
    TOP_LEVEL
    crm_bot/__init__.py
    crm_bot/app.py
    crm_bot/intents.py
    crm_bot/nlg_globals.py
    crm_bot/transition_model.py
    crm_bot/api/__init__.py
    crm_bot/api/crm_bot.py
)

END()

RECURSE_FOR_TESTS(ut)
