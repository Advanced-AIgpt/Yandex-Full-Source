PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __init__.py
    accesslog.py
    async_file_logger.py
    obscure.pyx
    sessionlog.py
    webhandlerlog.py
    log_filter.py
)

PEERDIR(
    alice/rtlog/client/python/lib

    alice/uniproxy/library/backends_common
    alice/uniproxy/library/backends_tts
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/logging
    alice/uniproxy/library/utils

    contrib/python/tornado/tornado-4
)

END()

RECURSE_FOR_TESTS(ut)
