PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __init__.py
    action_counter.py
    decorators.py
    deepcopyx.pyx
    deepupdate.py
    dict_object.py
    experiments.py
    futures.py
    graph_overrides.py
    hostname.py
    json_to_proto.py
    proto_to_json.py
    security.py
    srcrwr.py
    timestamp.py
    tree.py
    wshelpers.py
)

PEERDIR(
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/logging
    alice/uniproxy/library/settings
    contrib/python/tornado/tornado-4
    contrib/python/protobuf
    voicetech/library/proto_api
)

END()

RECURSE_FOR_TESTS(ut)
