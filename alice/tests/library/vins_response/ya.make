PY3_LIBRARY()

OWNER(mihajlova)

PY_SRCS(
    __init__.py
    analytics_info.py
    hollywood_response.py
    response.py
    vins_response.py

    div_card/__init__.py
    div_card/action.py
    div_card/div_card.py
    div_card/div2_card.py
)

PEERDIR(
    alice/tests/library/scenario
    contrib/python/cached-property
)

END()
