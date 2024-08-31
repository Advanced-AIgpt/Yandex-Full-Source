#!/usr/bin/env python
# -*- coding: utf-8 -*-

from collections import Counter

from nile.api.v1 import Record
from qb2.api.v1 import typing as qt

from utils.common import mapper_wrapper
from utils.tv.app_version import parse_app_version

from dayuse_utils import (
    get_counter_most_common_element,
    get_set_not_null_elements,
    dict_to_list,
    increment_path,
)
from constants import (
    SCREENS,
    UNDEFINED,
)
from screens_info import (
    ScreensInfo,
)

from logins_info import (
    LoginsInfo,
)

from apps_timespent import (
    AppsTimespent,
)

from carousels_info import (
    CarouselsInfo,
)

from hdmi_info import (
    HdmiInfo,
)


@mapper_wrapper
class DayuseReducer(object):
    output_schema = dict(
        device_id=qt.Optional[qt.String],
        eth0=qt.Optional[qt.String],
        wlan0=qt.Optional[qt.String],
        mac_id=qt.Optional[qt.String],
        fielddate=qt.Optional[qt.String],
        uuid=qt.Optional[qt.String],
        puid=qt.Optional[qt.String],
        app_version=qt.Optional[qt.String],
        board=qt.Optional[qt.String],
        platform=qt.Optional[qt.String],
        firmware_version=qt.Optional[qt.String],
        build=qt.Optional[qt.String],
        build_fingerprint=qt.Optional[qt.String],
        manufacturer=qt.Optional[qt.String],
        model=qt.Optional[qt.String],
        geo_id=qt.Optional[qt.Int64],
        session_ids=qt.Optional[qt.Yson],
        is_logged_in=qt.Optional[qt.Bool],
        has_leased_module=qt.Optional[qt.Bool],
        has_plus=qt.Optional[qt.Bool],
        has_paid_plus=qt.Optional[qt.Bool],
        active_tv_gift=qt.Optional[qt.Bool],
        active_tv_gift_plus=qt.Optional[qt.Bool],
        active_other_gift=qt.Optional[qt.Bool],
        active_other_trial=qt.Optional[qt.Bool],
        potential_gift=qt.Optional[qt.Bool],
        clid1=qt.Optional[qt.String],
        diagonal=qt.Optional[qt.Float],
        resolution=qt.Optional[qt.String],
        is_factory_ip=qt.Optional[qt.Bool],
        test_buckets=qt.Optional[qt.String],
        hdmi_timespent=qt.Optional[qt.Yson],
        carousels_info=qt.Optional[qt.Yson],
        cards_info=qt.Optional[qt.Yson],
        screens_info=qt.Optional[qt.Yson],
        quasar_device_id=qt.Optional[qt.String],
        puids=qt.Optional[qt.Yson],
        pluses=qt.Optional[qt.Yson],
        gifts=qt.Optional[qt.Yson],
        apps_timespent=qt.Optional[qt.Yson],
        tandem_connection_state=qt.String,
        tandem_info=qt.Yson,
    )

    def __call__(self, groups):
        for key, records in groups:
            results = {}

            counter_nodes = [
                "uuid",
                "puid",
                "manufacturer",
                "model",
                "geo_id",
                "clid1",
                "diagonal",
                "resolution",
                "is_factory_ip",
                "eth0",
                "wlan0",
                "mac_id",
                "quasar_device_id",
            ]
            counter_fields = {i: Counter() for i in counter_nodes}

            last_timestamp_fields = {
                "app_version": None,
                "build_fingerprint": None,
            }

            bool_other_fields = [
                "is_logged_in",
                "has_leased_module",
                "has_plus",
                "has_paid_plus",
                "active_tv_gift",
                "active_tv_gift_plus",
                "active_other_gift",
                "active_other_trial",
                "potential_gift",
            ]

            other_fields = {
                "session_ids": set(),
                "test_buckets": dict(),
                "tandem_connection_state": None
            }
            for field in bool_other_fields:
                other_fields[field] = set()

            screens_info = ScreensInfo()
            cards_info = {}

            logins_info = LoginsInfo()

            carousels_info = CarouselsInfo()
            apps_timespent = AppsTimespent()

            hdmi_info = HdmiInfo()

            tandem_info = {}

            for rec in records:
                screens_info.add_event(rec)
                logins_info.add_event(rec)
                carousels_info.add_event(rec)
                apps_timespent.add_event(rec)
                hdmi_info.add_event(rec)

                for field in counter_fields:
                    if rec[field] is not None:
                        counter_fields[field][rec[field]] += 1

                for field in last_timestamp_fields:
                    if rec[field] is not None:
                        last_timestamp_fields[field] = rec[field]

                if rec["event_name"] in ["card_click", "card_show"]:
                    screen = rec["event_value"].get("place", UNDEFINED)
                    carousel_name = rec["event_value"].get("carousel_name", UNDEFINED)
                    card_position = rec["event_value"].get("x")
                    carousel_position = rec["event_value"].get("y")

                    if SCREENS.get(screen, {}).get("type") == "outer":
                        cards_info = increment_path(
                            cards_info,
                            [
                                screen,
                                carousel_name,
                                carousel_position,
                                card_position,
                                "click"
                                if rec["event_name"] == "card_click"
                                else "show",
                            ],
                            1,
                        )

                other_fields["session_ids"].add(rec.session_id)
                for field in bool_other_fields:
                    other_fields[field].add(rec[field])

                if other_fields["tandem_connection_state"] != "1":
                    other_fields["tandem_connection_state"] = rec.get("tandem_connection_state")

                if rec["tandem_device_id"] is not None:
                    tandem_info.setdefault(rec["tandem_device_id"], "0")
                    if tandem_info[rec["tandem_device_id"]] != "1":
                        tandem_info[rec["tandem_device_id"]] = rec.get("tandem_connection_state")

                if rec.buckets is not None:
                    for test_bucket in rec.buckets:
                        test_id, bucket_id = str(test_bucket[0]), str(test_bucket[1])
                        other_fields["test_buckets"][test_id] = bucket_id

            for field in counter_fields:
                results[field] = get_counter_most_common_element(counter_fields[field])

            for field in last_timestamp_fields:
                results[field] = last_timestamp_fields[field]

            parsed_app_version = parse_app_version(last_timestamp_fields["app_version"])

            results["board"] = parsed_app_version.get("board")
            results["platform"] = parsed_app_version.get("platform")
            results["firmware_version"] = parsed_app_version.get("firmware_version")
            results["build"] = parsed_app_version.get("build")

            results["session_ids"] = get_set_not_null_elements(
                other_fields["session_ids"]
            )
            for field in bool_other_fields:
                results[field] = max(other_fields[field]) or False
            results["tandem_connection_state"] = other_fields["tandem_connection_state"] or ""

            results["test_buckets"] = (
                ";".join(
                    map(
                        lambda x: ",".join([x[0], "0", x[1]]),
                        other_fields["test_buckets"].items(),
                    )
                )
                or None
            )

            results["hdmi_timespent"] = [
                (device_name, port, timespent)
                for (device_name, port), timespent in hdmi_info.get_info().items()
            ]

            results["carousels_info"] = dict_to_list(carousels_info.get_info())
            results["cards_info"] = dict_to_list(cards_info)
            results["screens_info"] = dict_to_list(screens_info.get_info())

            results["puids"] = logins_info.get_puids()
            results["pluses"] = logins_info.get_pluses()
            results["gifts"] = logins_info.get_gifts()

            results["tandem_info"] = tandem_info

            results["apps_timespent"] = apps_timespent.get_info()

            yield Record(key, **results)
