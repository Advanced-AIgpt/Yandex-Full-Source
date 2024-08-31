PY3_LIBRARY(request_generator)

OWNER(
    g:alice_downloaders
)

PEERDIR(
    contrib/python/attrs
    contrib/python/deepmerge
    contrib/python/pytz
    contrib/python/requests

    alice/megamind/protos/common
)

PY_SRCS(
    app_presets.py
    helpers.py
    state_preset.py
    uniproxy.py
    vins.py
    retries_profiles.py
)

END()
