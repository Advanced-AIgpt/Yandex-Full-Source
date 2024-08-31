PY3TEST()

OWNER(
    g:voicetech-infra
)

TEST_SRCS(
    uaas_mapper_ut.py
)

PEERDIR(
    alice/uniproxy/library/uaas_mapper
)

END()
