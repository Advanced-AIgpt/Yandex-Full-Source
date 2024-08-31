PY3_LIBRARY()
OWNER(g:voicetech-infra)

PEERDIR(
    contrib/python/tornado/tornado-4

    alice/uniproxy/library/async_http_client
    alice/uniproxy/library/backends_bio
    alice/uniproxy/library/backends_common
    alice/uniproxy/library/events
    alice/uniproxy/library/extlog
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/logging
    alice/uniproxy/library/perf_tester
    alice/uniproxy/library/personal_data
    alice/uniproxy/library/settings
    alice/uniproxy/library/utils
)

PY_SRCS(
    __init__.py
    fake_stream.py
    vinsadapter.py
    vinsrequest.py
    validation.py
)

END()

RECURSE_FOR_TESTS(ut)
