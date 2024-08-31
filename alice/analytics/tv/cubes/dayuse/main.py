#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
from datetime import timedelta
from nile.api.v1 import (
    extractors as ne,
    datetime as nd,
)

from utils.nirvana.op_caller import call_as_operation
from utils.yt.dep_manager import hahn_with_deps

from utils.common import get_dict_path, set_last_table_on_finish


from qb2.api.v1 import (
    typing as qt,
    filters as qf,
    extractors as qe,
)

from dayuse_reducer import (
    DayuseReducer,
)

from constants import (
    TVANDROID_SESSIONS_DIR,
    TVANDROID_SESSIONS_PATH,
    ACTIVATIONS_CUBE_DIR,
    ACTIVATIONS_CUBE_PATH,
    CRYPTA_DEVICE_ID_CID_PATH,
    CRYPTA_PUID_CID_PATH,
    TVANDROID_DAYUSE_CUBE_DIR,
    DAYS_UPDATE_PERIOD,
    TVANDROID_SESSIONS_FIELDS,
    HDMI_EVENTS,
    DAYS_RECALC_PERIOD,
)

from hdmi_fields import (
    get_hdmi_fields,
    join_hdmi_fields,
)

from daily_fields import (
    get_daily_fields,
    join_daily_fields,
)

from ads_fields import (
    get_ads_delay_duration_fields,
    join_ads_delay_duration_fields,
)

from dayuse_utils import (
    is_new,
)


def join_dayuse_with_crypta(job, dayuse_cube):
    crypta_device_id_cid = job.table(CRYPTA_DEVICE_ID_CID_PATH).project(
        device_id="id", crypta_id="target_id"
    )

    crypta_puid_cid = job.table(CRYPTA_PUID_CID_PATH).project(
        puid="id", crypta_id="target_id"
    )

    dayuse_cube_wo_puid, dayuse_cube_with_puid = dayuse_cube.split(qf.defined("puid"))

    dayuse_cube_wo_puid_with_crypta = dayuse_cube_wo_puid.join(
        crypta_device_id_cid, by="device_id", type="left", assume_unique_right=True
    )

    dayuse_cube_with_puid_with_crypta = dayuse_cube_with_puid.join(
        crypta_puid_cid, by="puid", type="left", assume_unique_right=True
    )
    dayuse_cube_with_crypta = job.concat(
        dayuse_cube_wo_puid_with_crypta, dayuse_cube_with_puid_with_crypta
    )

    return dayuse_cube_with_crypta


def make_job(
    job,
    date,
    days_update_period=DAYS_UPDATE_PERIOD,
    days_recalc_period=DAYS_RECALC_PERIOD,
    dayuse_cube_dir=TVANDROID_DAYUSE_CUBE_DIR,
    tvandroid_sessions_dir=TVANDROID_SESSIONS_DIR,
    activations_cube_dir=ACTIVATIONS_CUBE_DIR,
    dont_use_existing_cube=False,
):

    update_period_start_date = nd.next_day(
        date, offset=-int(days_update_period), scale="daily"
    )

    recalc_period_start_date = nd.next_day(
        update_period_start_date,
        offset=-int(days_recalc_period),
        scale="daily",
    )

    job = job.env(
        templates={
            "dayuse_cube_dir": dayuse_cube_dir,
            "tvandroid_sessions_dir": tvandroid_sessions_dir,
            "activations_cube_dir": activations_cube_dir,
        },
    )

    activations = job.table(ACTIVATIONS_CUBE_PATH).label("activations").filter(
        qf.compare("activation_date", "<=", date),
    ).project(
        "activation_id",
        "activation_date"
    )

    raw_tvandroid_sessions_log = job.table(
        TVANDROID_SESSIONS_PATH.format(
            start_date=recalc_period_start_date, end_date=date
        )
    ).label("input")

    tvandroid_sessions_log = raw_tvandroid_sessions_log.filter(
        qf.compare("event_date", ">=", update_period_start_date)
    ).project(
        *TVANDROID_SESSIONS_FIELDS,
        fielddate="event_date",
        clid1=ne.custom(
            lambda clids: get_dict_path(clids, ["clid1"], convert_type=str), "clids"
        ).with_type(qt.Optional[qt.String]),
        client_timestamp_ms=ne.custom(
            lambda event_value: get_dict_path(
                event_value, ["client_timestamp_ms"], default=0
            ),
            "event_value",
        ).with_type(qt.Optional[qt.Int64]),
        hdmi_session_id=ne.custom(
            lambda event_name, event_value: event_value.get("hdmi_session_id")
            if event_name in HDMI_EVENTS
            else None,
            "event_name",
            "event_value",
        ).with_type(qt.Optional[qt.String])
    )

    hdmi_session_info = get_hdmi_fields(raw_tvandroid_sessions_log)

    tvandroid_sessions_log = join_hdmi_fields(
        tvandroid_sessions_log, hdmi_session_info
    )

    dayuse_cube_current = (
        tvandroid_sessions_log.groupby("device_id", "fielddate")
        .sort("event_timestamp", "client_timestamp_ms")
        .reduce(DayuseReducer())
    )

    day_info = get_daily_fields(raw_tvandroid_sessions_log)

    ads_delay_duration = get_ads_delay_duration_fields(raw_tvandroid_sessions_log)

    dayuse_cube_current = join_ads_delay_duration_fields(dayuse_cube_current, ads_delay_duration)

    dayuse_cube_current = (
        join_daily_fields(dayuse_cube_current, day_info)
        .project(
            ne.all(),
            qe.or_("activation_id", "eth0", "quasar_device_id").with_type(
                qt.Optional[qt.String]
            ),
        )
        .join(activations, by="activation_id", type="left", assume_unique_right=True)
        .project(
            ne.all(),
            is_new=ne.custom(is_new, "activation_date", "fielddate").with_type(
                qt.Optional[qt.Bool]
            )
        )
        .label("output")
    )

    if dont_use_existing_cube:
        dayuse_cube = dayuse_cube_current
    else:
        dayuse_cube_past = job.table(
            os.path.join(TVANDROID_DAYUSE_CUBE_DIR, "last")
        ).filter(qf.compare("fielddate", "<", update_period_start_date))

        dayuse_cube_future = job.table(
            os.path.join(TVANDROID_DAYUSE_CUBE_DIR, "last")
        ).filter(qf.compare("fielddate", ">", date))

        dayuse_cube = job.concat(
            dayuse_cube_past, dayuse_cube_current, dayuse_cube_future
        ).project(ne.all(exclude="crypta_id"))

    dayuse_cube_with_crypta = join_dayuse_with_crypta(job, dayuse_cube)

    dayuse_cube_with_crypta.sort(
        "fielddate", "manufacturer", "model", "app_version", "device_id"
    ).put("$dayuse_cube_dir/{}".format(date), ttl=timedelta(7))

    return job


@set_last_table_on_finish("date", "dayuse_cube_dir", TVANDROID_DAYUSE_CUBE_DIR)
def main(*args, **kwargs):
    cluster = hahn_with_deps(
        pool="smarttv",
        neighbours_for=__file__,
        include_utils=True,
        use_yql=True,
        custom_cluster_params={"yql_token_for": {"yt"}},
    )
    job = cluster.job()
    job = make_job(job, *args, **kwargs)
    job.run()
    return job


if __name__ == "__main__":
    call_as_operation(main)
    # How to run
    # Setup virtualenv:
    # virtualenv ~/env/analytics -p python
    # source ~/env/analytics/bin/activate

    # Install nile, qb2, fire
    # pip install -i https://pypi.yandex-team.ru/simple nile
    # pip install -i https://pypi.yandex-team.ru/simple qb2
    # pip install fire

    # Install analytics as a package
    # cd ~/arc/arcadia/alice/analytics
    # pip install -e .

    # Launch this script with parameters
    # python main.py --date 2021-01-01
