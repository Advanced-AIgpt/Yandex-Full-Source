PY2TEST()

OWNER(g:alice)

IF(AUTOCHECK)
    TIMEOUT(600)

    SIZE(MEDIUM)

    REQUIREMENTS(
        network:full
        cpu:4
        ram:11
    )

    FORK_SUBTESTS()

    SPLIT_FACTOR(20)

ELSE()
    TIMEOUT(10000)

    TAG(ya:not_autocheck)

ENDIF()

SRCDIR(alice/vins/apps/personal_assistant)

PEERDIR(
    alice/vins/apps/personal_assistant/ut/functional
)

INCLUDE(${ARCADIA_ROOT}/alice/vins/tests_env.inc)

DEPENDS(alice/vins/resources)

RESOURCE(
    app_presets.yaml /load_app_infos/app_presets.yaml
)

END()
