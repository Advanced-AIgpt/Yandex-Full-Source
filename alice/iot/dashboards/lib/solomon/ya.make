OWNER(g:alice_iot)

PY3_LIBRARY()

PEERDIR(
    contrib/python/attrs
    contrib/python/requests
)

PY_SRCS(
    alert_builder.py
    aql.py
    bulbasaur.py
    dashboard_builder.py
    graphic_builder.py
    provider.py
    queue_graphics.py
    service_graphics.py
    time_machine.py
    tuya.py
    unified_agent.py
    uxie.py
    vulpix.py
    xiaomi.py
)

END()
