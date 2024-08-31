OWNER(ferume-oh3)

PY23_LIBRARY()

SRCDIR(alice/analytics/utils/yt)

PY_SRCS(
    NAMESPACE utils.yt
    extract_intent.py
    extract_asr.py
    listing.py
    basket_common.py
    dep_manager.py
)

PEERDIR(
    contrib/python/pytz
    alice/analytics/utils
)

END()
