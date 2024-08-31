from alice.cuttlefish.library.python.evcheck import EventCheck


event_check = EventCheck()


def test_good():
    assert event_check.check(
        b"""{"event": {
        "header": {
            "name": "SynchronizeState",
            "namespace": "System"
        },
        "payload": {
            "auth_token": "c8a25150-0915-453f-9f58-eccf51885e1c",
            "device": "samsung SM-J320F",
            "device_manufacturer": "samsung",
            "device_model": "SM-J320F",
            "network_type": "MOBILE:HSPA+:CONNECTED",
            "platform_info": "android",
            "speechkitVersion": "3.13.0",
            "uuid": "cf7403caabf93cbaf014327e2ca2cddc",
            "vins": {
                "application": {
                    "app_id": "ru.yandex.taximeter",
                    "app_version": "9.17",
                    "device_manufacturer": "samsung",
                    "device_model": "SM-J320F",
                    "os_version": "5.1.1",
                    "platform": "android",
                    "uuid": "cf7403caabf93cbaf014327e2ca2cddc"
                }
            },
            "yandexuid": ""
        }
    }}"""
    )


def test_bad():
    assert not event_check.check(
        b"""{"event": {
        "header": {
            "name": "SynchronizeState",
            "namespace": "System"
        },
        "payload": {
            "auth_token": "c8a25150-0915-453f-9f58-eccf51885e1c",
            "device": "samsung SM-J320F",
            "device_manufacturer": "samsung",
            "device_model": "SM-J320F",
            "network_type": "MOBILE:HSPA+:CONNECTED",
            "platform_info": "android",
            "speechkitVersion": "3.13.0",
            "uuid": "cf7403caabf93cbaf014327e2ca2cddc",
            "vins": {
                "application": {
                    "app_id_X": "ru.yandex.taximeter",
                    "app_version": "9.17",
                    "device_manufacturer": "samsung",
                    "device_model": "SM-J320F",
                    "os_version": "5.1.1",
                    "platform": "android",
                    "uuid": "cf7403caabf93cbaf014327e2ca2cddc"
                }
            },
            "yandexuid": ""
        }
    }}"""
    )
