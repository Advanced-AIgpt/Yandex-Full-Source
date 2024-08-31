PY2TEST()

OWNER(g:alice)

IF(AUTOCHECK)
    TIMEOUT(600)

    SIZE(MEDIUM)

    REQUIREMENTS(
        network:full
        cpu:4
    )

    FORK_SUBTESTS()

    SPLIT_FACTOR(100)

ELSE()
    TIMEOUT(10000)

    TAG(ya:not_autocheck)

ENDIF()


PEERDIR(
    alice/vins/apps/personal_assistant/ut/lib
)

SRCDIR(alice/vins/apps/personal_assistant)

INCLUDE(${ARCADIA_ROOT}/alice/vins/tests_env.inc)

DEPENDS(alice/vins/resources)

TEST_SRCS(
    personal_assistant/tests/test_bass_api.py
    personal_assistant/tests/test_bass_client.py
    personal_assistant/tests/test_dialog.py
    personal_assistant/tests/test_general_conversation.py
    personal_assistant/tests/test_general_conversation_topic.py
    personal_assistant/tests/test_hardcoded.py
    personal_assistant/tests/test_intents.py
    personal_assistant/tests/test_item_selection.py
    personal_assistant/tests/test_nlg_filters.py
    personal_assistant/tests/test_nlg_globals.py
    personal_assistant/tests/test_personal_assistant.py
    personal_assistant/tests/test_setup_features.py
    personal_assistant/tests/test_stroka.py
    personal_assistant/tests/test_testing_framework.py
    personal_assistant/tests/test_transition_model.py
    personal_assistant/tests/test_unbroken_tests.py
    personal_assistant/tests/test_update_form.py
    personal_assistant/tests/test_is_allowed_intent/test_resolver.py
    personal_assistant/tests/test_is_allowed_intent/__init__.py
    personal_assistant/tests/test_is_allowed_intent/predicates/predicates1.py
    personal_assistant/tests/test_is_allowed_intent/predicates/__init__.py
)

END()
