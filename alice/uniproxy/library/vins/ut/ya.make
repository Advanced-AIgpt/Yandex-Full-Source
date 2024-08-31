PY3TEST()
OWNER(g:voicetech-infra)

PEERDIR(
    contrib/python/tornado/tornado-4
    alice/uniproxy/library/events
    alice/uniproxy/library/vins
    alice/uniproxy/library/processors
    alice/uniproxy/library/testing
)

TEST_SRCS(
    test_vins_adapter.py
    test_vins_adapter_with_delayed_future.py
    test_vins_prepare_request_counters.py
    test_vins_request_fallback_parts.py
    test_vins_timings.py
    test_vins_url_mapping.py
    test_vins_validation.py
)

END()
