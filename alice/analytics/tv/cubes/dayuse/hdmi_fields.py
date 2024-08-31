from qb2.api.v1 import (
    typing as qt,
    filters as qf,
)

from nile.api.v1 import (
    extractors as ne,
)


def get_hdmi_fields(events):
    """Returns hdmi sessions info: device name and port."""
    return (
        events.filter(qf.equals("event_name", "hdmi_opened"))
        .project(
            "device_id",
            hdmi_session_id=ne.custom(
                lambda x: x.get("hdmi_session_id"), "event_value"
            ).with_type(qt.Optional[qt.String]),
            hdmi_session_device_name=ne.custom(
                lambda x: x.get("device_name", "no_device"), "event_value"
            ).with_type(qt.String),
            hdmi_session_port=ne.custom(
                lambda x: x.get("port", "no_port"), "event_value"
            ).with_type(qt.String),
        )
        .unique("device_id", "hdmi_session_id")
    )


def join_hdmi_fields(tvandroid_sessions, hdmi_session_info):
    return tvandroid_sessions.join(
        hdmi_session_info,
        by=["device_id", "hdmi_session_id"],
        type="left",
        assume_unique_right=True,
    ).project(
        ne.all(exclude=["hdmi_session_id"]),
        hdmi_session_device_name=ne.custom(
            lambda x: "undefined_device" if x is None else x, "hdmi_session_device_name"
        ).with_type(qt.String),
        hdmi_session_port=ne.custom(
            lambda x: "undefined_port" if x is None else x, "hdmi_session_port"
        ).with_type(qt.String),
    )
