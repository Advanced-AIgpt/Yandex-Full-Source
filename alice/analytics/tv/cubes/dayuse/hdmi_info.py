#!/usr/bin/env python
# -*- coding: utf-8 -*-

from dayuse_utils import (
    get_device_identifier,
    seconds_between,
)


class HdmiInfo(object):
    def __init__(self):
        self.hdmi_timespent = {}
        self.hdmi_session_timespent = {}
        self.hdmi_session_id = None
        self.hdmi_heartbeat_datetime = None

    @staticmethod
    def edit_hdmi_session_timespent(
        hdmi_session_timespent,
        device_identifier,
        event_datetime,
        timespent,
        append=True,
    ):
        if timespent >= 0 and not append:
            hdmi_session_timespent[device_identifier] = min(
                timespent, seconds_between(event_datetime)
            )
        else:
            if device_identifier not in hdmi_session_timespent:
                hdmi_session_timespent[device_identifier] = 0
            hdmi_session_timespent[device_identifier] += max(timespent, 0)
        return hdmi_session_timespent

    @staticmethod
    def update_hdmi_timespent(hdmi_timespent, hdmi_session_timespent):
        for device in hdmi_session_timespent:
            if device in hdmi_timespent:
                hdmi_timespent[device] += hdmi_session_timespent[device]
            else:
                hdmi_timespent[device] = hdmi_session_timespent[device]
        return hdmi_timespent

    def add_event(self, record):
        if record["event_name"] in [
            "hdmi_opened",
            "hdmi_heartbeat",
            "hdmi_session_info",
        ]:
            device_identifier = get_device_identifier(record)
            cur_hdmi_session_id = record["event_value"].get(
                "hdmi_session_id", "undefined_session_id"
            )
            timespent = 0
            append = None

            if self.hdmi_session_id != cur_hdmi_session_id:
                self.hdmi_timespent = self.update_hdmi_timespent(
                    self.hdmi_timespent, self.hdmi_session_timespent
                )
                self.hdmi_session_id = cur_hdmi_session_id
                self.hdmi_session_timespent = {}

            if record["event_name"] == "hdmi_heartbeat":
                append = True
                # By default heartbeats are coming every 30 sec
                hdmi_heartbeat_period = record["event_value"].get("period", 30)
                if self.hdmi_heartbeat_datetime is None:
                    timespent = hdmi_heartbeat_period
                else:
                    timespent = min(
                        hdmi_heartbeat_period,
                        seconds_between(
                            self.hdmi_heartbeat_datetime, record["event_datetime"]
                        ),
                    )
                self.hdmi_heartbeat_datetime = record["event_datetime"]

            elif record["event_name"] == "hdmi_session_info":
                append = False
                timespent = record["event_value"].get("time_spent", -1)

            self.hdmi_session_timespent = self.edit_hdmi_session_timespent(
                self.hdmi_session_timespent,
                device_identifier,
                record["event_datetime"],
                timespent,
                append,
            )

    def get_info(self):
        self.hdmi_timespent = self.update_hdmi_timespent(
            self.hdmi_timespent, self.hdmi_session_timespent
        )
        return dict(self.hdmi_timespent)
