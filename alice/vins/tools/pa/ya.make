PY2_PROGRAM(pa_tools)

OWNER(g:alice)

INCLUDE(${ARCADIA_ROOT}/alice/vins/apps/personal_assistant/ut/mm_resources.inc)

PEERDIR(
    contrib/python/attrs
    contrib/python/color
    contrib/python/requests
    alice/vins/core
    alice/vins/apps/personal_assistant
    alice/vins/apps/personal_assistant/testing_framework
)

RESOURCE_FILES(
    PREFIX integration_test_viewer/
    integration_test_viewer/index.html
    integration_test_viewer/viewer.js
    integration_test_viewer/test.json
    integration_test_viewer/viewer.css
)

PY_SRCS(
    __main__.py
    download_bass_stubs.py
    run_integration_tests.py
    run_integration_diff_tests.py
)

END()
