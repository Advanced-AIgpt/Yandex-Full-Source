PY3TEST()

OWNER(
    g:matrix
)

PEERDIR(
    alice/matrix/notificator/tests/library

    library/python/cityhash
)

TEST_SRCS(
    test_delivery.py
    test_delivery_demo.py
    test_delivery_on_connect.py
    test_delivery_push.py
    test_devices.py
    test_directive.py
    test_gdpr.py
    test_locator.py
    test_notifications.py
    test_simple.py
    test_subscriptions.py
    test_update_connected_clients.py
    test_update_device_environment.py
)

INCLUDE(${ARCADIA_ROOT}/alice/matrix/notificator/tests/library/data.ya.make.inc)

INCLUDE(${ARCADIA_ROOT}/alice/matrix/library/testing/python/data.ya.make.inc)
INCLUDE(${ARCADIA_ROOT}/alice/matrix/library/testing/ydb_recipe/ya.make.inc)

INCLUDE(${ARCADIA_ROOT}/library/recipes/tvmapi/recipe.inc)
INCLUDE(${ARCADIA_ROOT}/library/recipes/tvmtool/recipe.inc)

USE_RECIPE(
    library/recipes/tvmtool/tvmtool
    alice/matrix/notificator/tests/library/tvmtool_config.json
)

SIZE(MEDIUM)
REQUIREMENTS(ram:32)

END()

RECURSE(
    library
)
