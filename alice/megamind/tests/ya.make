PY3TEST()

OWNER(g:megamind)

SIZE(LARGE)
TAG(ya:fat ya:force_sandbox)
REQUIREMENTS(
    sb_vault:VAULT_CLIENT=file:BASS:robot-bassist_vault_token
    network:full
)
TIMEOUT(3600)

TEST_SRCS(
    test_begemot.py
    test_configs.py
    test_smoke.py
    test_vins_like_log.py
    test_general_conversation.py
)

DATA(
    arcadia/alice/megamind/configs
)

PEERDIR(
    alice/megamind/tests/library
)

DEPENDS(
    alice/megamind/scripts/run
    alice/megamind/server
    alice/megamind/tools/config_validator
)

INCLUDE(${ARCADIA_ROOT}/alice/joker/library/python/for_tests.inc)

END()

RECURSE(library)
