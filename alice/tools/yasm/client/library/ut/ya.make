PY3TEST()

STYLE_PYTHON()

OWNER(g:voicetech-infra)

RESOURCE(
    data/objects.json /common/objects.json
    data/naming_data.json /naming/data.json
    data/naming_results.json /naming/results.json
)

FORK_SUBTESTS()

TEST_SRCS(naming_ut.py)

PEERDIR(
    alice/tools/yasm/client/library
    library/python/resource
)

END()
