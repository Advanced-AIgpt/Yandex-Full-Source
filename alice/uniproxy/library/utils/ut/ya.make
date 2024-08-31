PY3TEST()

OWNER(
    g:voicetech-infra
)

TEST_SRCS(
    deepcopy_ut.py
    deepsetdefault_ut.py
    dict_object_ut.py
    hostname_ut.py
    security_ut.py
    srcrwr_ut.py
    experiments_ut.py
    tree_ut.py
    proto_to_json_ut.py
    json_to_proto_ut.py
)

PEERDIR(
    alice/uniproxy/library/utils
    mssngr/router/lib/protos
)


END()
