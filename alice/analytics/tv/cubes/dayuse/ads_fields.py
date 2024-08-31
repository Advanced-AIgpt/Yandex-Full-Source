from qb2.api.v1 import (
    typing as qt,
    filters as qf,
)

from nile.api.v1 import (
    extractors as ne,
    aggregators as na,
)

from utils.common import get_dict_path

from constants import ADS_INVALID_DURATION


def get_ads_delay_duration_fields(events):
    """Returns ads delay durations."""
    return (
        events.filter(qf.equals("event_name", "ad_preroll_start_delay"))
        .project(
            "device_id",
            fielddate="event_date",
            ads_delay_duration_ms=ne.custom(
                lambda event_value: get_dict_path(
                    event_value, ["duration_ms"], default=0
                ),
                "event_value",
            ).with_type(qt.Optional[qt.Int64]),
        )
        .groupby("device_id", "fielddate")
        .aggregate(
            ads_delay_duration_ms=na.sum(
                "ads_delay_duration_ms",
                predicate=qf.not_(
                    qf.equals("ads_delay_duration_ms", ADS_INVALID_DURATION)
                ),
            ),
            ads_preroll_count=na.count()
        )
    )


def join_ads_delay_duration_fields(dayuse_cube, ads_delay_duration_info):
    return dayuse_cube.join(
        ads_delay_duration_info,
        by=["device_id", "fielddate"],
        type="left",
        assume_unique_right=True,
    )
