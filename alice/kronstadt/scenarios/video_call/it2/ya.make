PY3TEST()

OWNER(
    g:paskills
)
SIZE(medium)
DEPENDS(${ARCADIA_ROOT}/alice/kronstadt/scenarios/video_call/it2/shard)

INCLUDE(${ARCADIA_ROOT}/alice/kronstadt/it2/common.inc)
NO_CHECK_IMPORTS()

PEERDIR(
    alice/kronstadt/scenarios/video_call/src/main/proto
)

PY_SRCS(
    endpoint_state_updates.py
    env_states.py
    predefined_contacts.py
)

TEST_SRCS(
    conftest.py
    tests_call.py
#    tests_call_controls.py - enable after begemot release
#    tests_contact_book.py - postponed it
    tests_endpoint_update.py
    tests_login.py
    tests_main_screen.py
    tests_for_ue2e.py
)

END()

RECURSE(shard)
