PY3_LIBRARY()

OWNER(
    g:hollywood
    vitvlkv
)

PEERDIR(
    alice/library/python/utils
    apphost/lib/grpc/protos
    contrib/libs/protobuf
    contrib/python/falcon
    contrib/python/python-slugify
    contrib/python/requests
    infra/yp_service_discovery/api
    infra/yp_service_discovery/python/resolver
    library/python/codecs
    library/python/filelock
)

PY_SRCS(
    static_stubber.py
    stubber_config.proto
    stubber_server.py
)

END()

RECURSE_FOR_TESTS(
    tests
)
