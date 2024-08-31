PY3_LIBRARY()

OWNER(mihajlova)

PY_SRCS(
    __init__.py
    analytics.py
    event.py
    proto_wrapper.py
    scraper_uniclient.py
    serialization.py
    typed_semantic_frame.py
    uniclient.pyx
)

PEERDIR(
    alice/acceptance/modules/request_generator/scrapper/lib
    alice/library/client/protos
    alice/megamind/protos/common
    alice/megamind/protos/speechkit
    alice/uniproxy/mapper/uniproxy_client/lib
    contrib/python/attrs
    contrib/python/retry
    library/python/cyson
)

END()
