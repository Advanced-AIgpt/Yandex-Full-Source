PY3TEST()

OWNER(
    g:voicetech-infra
)

FORK_SUBTESTS()

TEST_SRCS(
    timezone_ut.py
    get_tags_ut.py
    tag_sanitize_ut.py
    json_serialize_ut.py
)

PEERDIR(
    alice/tools/qloud_format/lib
)

END()
