PY3_LIBRARY()

OWNER(mihajlova)

PY_SRCS(
    __init__.py
    alice.py
    device_state.py
    muzpult.py
    surface.py

    directives/__init__.py
    directives/mixin.py

    directives/automotive.py
    directives/browser.py
    directives/launcher.py
    directives/legatus.py
    directives/maps.py
    directives/navi.py
    directives/sdc.py
    directives/searchapp.py
    directives/smart_display.py
    directives/smart_tv.py
    directives/station.py
    directives/station_pro.py
    directives/taximeter.py
    directives/watch.py
    directives/webtouch.py
    directives/yabro_win.py
)

PEERDIR(
    alice/acceptance/modules/request_generator/lib
    alice/megamind/protos/common
    alice/protos/endpoint/capabilities/route_manager
    alice/tests/library/directives
    alice/tests/library/uniclient
    alice/tests/library/vault
    alice/tests/library/vins_response
    contrib/python/icalendar
    contrib/python/pytz
    contrib/python/retry
)

END()
