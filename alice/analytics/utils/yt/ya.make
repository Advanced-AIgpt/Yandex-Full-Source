OWNER(g:alice_analytics)

PY23_LIBRARY()

PEERDIR(
    alice/analytics/utils
)

PY_SRCS(
    extract_intent.py
    extract_asr.py
    listing.py
    basket_common.py
    dep_manager.py
)

END()
