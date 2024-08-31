from datetime import timedelta

from qb2.api.v1 import (
    typing as qt,
    filters as qf,
)

from nile.api.v1 import (
    extractors as ne,
    aggregators as na,
)

from utils.common import (
    get_dict_path,
)

from dayuse_utils import (
    get_date_clamp_time,
    to_datetime,
)


def get_daily_fields(events):
    """Returns day info: screen time, screen saver time and apps versions."""
    events_nonzero_session_id = events.filter(qf.nonzero("session_id"))

    session_ids = events_nonzero_session_id.unique(
        "event_date", "device_id", "session_id", keep_only_group_fields=True
    )

    session_info = events_nonzero_session_id.groupby(
        "device_id", "session_id"
    ).aggregate(
        session_start_datetime=na.min("event_datetime"),
        session_end_datetime=na.max("event_datetime"),
        session_init_event_value=na.any(
            "event_value", predicate=qf.equals("event_name", "session_init")
        ),
    )

    screen_saver_info = events_nonzero_session_id.filter(
        qf.equals("event_name", "screensaver_stats")
    ).project(
        "device_id",
        "session_id",
        screen_saver_end_datetime="event_datetime",
        screen_saver_time=ne.custom(
            lambda event_value: get_dict_path(
                event_value, ["period"], default=0, convert_type=int
            ),
            "event_value",
        ).with_type(qt.Int64),
        screen_saver_start_datetime=ne.custom(
            lambda screen_saver_end_datetime, screen_saver_time: str(
                to_datetime(screen_saver_end_datetime)
                - timedelta(seconds=screen_saver_time)
            ),
            "screen_saver_end_datetime",
            "screen_saver_time",
        ).with_type(qt.String),
    )

    screen_saver_time = (
        session_ids.join(screen_saver_info, by=["device_id", "session_id"], type="left")
        .project(
            "event_date",
            "device_id",
            screen_saver_time=ne.custom(
                get_date_clamp_time,
                "event_date",
                "screen_saver_start_datetime",
                "screen_saver_end_datetime",
            ).with_type(qt.Int64),
        )
        .groupby("event_date", "device_id")
        .aggregate(screen_saver_time=na.sum("screen_saver_time"))
    )

    return (
        session_ids.join(
            session_info,
            by=["device_id", "session_id"],
            type="left",
            assume_unique_right=True,
        )
        .project(
            "event_date",
            "device_id",
            "session_init_event_value",
            "session_start_datetime",
            screen_time=ne.custom(
                get_date_clamp_time,
                "event_date",
                "session_start_datetime",
                "session_end_datetime",
            ).with_type(qt.Int64),
        )
        .groupby("event_date", "device_id")
        .aggregate(
            screen_time=na.sum("screen_time"),
            session_init_event_value=na.last(
                "session_init_event_value", "session_start_datetime"
            ),
        )
        .project(
            "event_date",
            "device_id",
            "screen_time",
            apps_versions=ne.custom(
                lambda event_value: get_dict_path(event_value, ["apps_versions"]),
                "session_init_event_value",
            ).with_type(qt.Optional[qt.Yson]),
        )
        .join(
            screen_saver_time,
            by=["event_date", "device_id"],
            type="left",
            assume_unique_right=True,
        )
        .project(ne.all(exclude=["event_date"]), fielddate="event_date")
    )


def join_daily_fields(log, session_info):
    return log.join(
        session_info,
        by=["fielddate", "device_id"],
        type="left",
        assume_unique_right=True,
    )
