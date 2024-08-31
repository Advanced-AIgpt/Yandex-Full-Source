PY3TEST()

OWNER(zubchick)

TEST_SRCS(
    test.py
    test_state_presets.py
    test_supported_features.py
    test_environment_state.py
)

PEERDIR(
    contrib/python/pytest
    contrib/python/pytest-mock
    contrib/python/freezegun
    contrib/python/PyHamcrest

    alice/acceptance/modules/request_generator/lib
)

DEPENDS(
    smart_devices/platforms/yandexmicro/config
    smart_devices/platforms/yandexmini/config
    smart_devices/platforms/yandexmini_2/config
    smart_devices/platforms/yandexstation/config
    smart_devices/platforms/yandexstation_2/config
)

SIZE(SMALL)

ENV(TEST_LOGNAME=pytest)
ENV(TEST_FQDN=pytest.local)

END()
