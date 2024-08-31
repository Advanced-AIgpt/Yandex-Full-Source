PY3_LIBRARY()
OWNER(g:voicetech-infra)

PEERDIR(
    contrib/python/tornado/tornado-4
    contrib/python/objgraph
    library/python/svn_version
    voicetech/library/proto_api

    alice/uniproxy/library/backends_asr
    alice/uniproxy/library/backends_common
    alice/uniproxy/library/backends_tts
    alice/uniproxy/library/common_handlers
    alice/uniproxy/library/events
    alice/uniproxy/library/experiments
    alice/uniproxy/library/frontend
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/logging
    alice/uniproxy/library/settings
    alice/uniproxy/library/unisystem
    alice/uniproxy/library/utils
)

PY_SRCS(
    __init__.py
    asrsocket.py
    experiment.py
    frontend.py
    loggingsocket.py
    memview.py
    monitoring.py
    postasrhandler.py
    revision.py
    ttssocket.py
)

END()

RECURSE_FOR_TESTS(ut)
