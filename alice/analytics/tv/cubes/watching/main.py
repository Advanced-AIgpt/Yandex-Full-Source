#!/usr/bin/env python
# -*- coding: utf-8 -*-

import re
import os
from datetime import timedelta

from nile.api.v1 import (
    aggregators as na,
    extractors as ne,
    filters as nf,
    datetime as nd,
)

from qb2.api.v1 import typing as qt, filters as qf


from utils.nirvana.op_caller import call_as_operation
from utils.yt.dep_manager import hahn_with_deps
from utils.common import get_dict_path, set_last_table_on_finish

from watching_utils import (
    get_appmetrica_view_type,
    get_strm_view_type,
    get_player_session_id,
    check_recommendation_watching,
    get_login_status,
    check_new_app_version,
)

from constants import (
    TVANDROID_SESSIONS_DIR,
    TVANDROID_SESSIONS_PATH,
    STRM_DIR,
    STRM_PATH,
    CONTENT_INFO_PATH,
    WATCHING_CUBE_DIR,
    DAYS_UPDATE_PERIOD,
    APPMETRICA_WATCHING_LOG_FIELDS,
    STRM_WATCHING_LOG_FIELDS,
)


def make_job(
    job,
    date,
    days_update_period=DAYS_UPDATE_PERIOD,
    tvandroid_sessions_dir=TVANDROID_SESSIONS_DIR,
    strm_dir=STRM_DIR,
    watching_cube_dir=WATCHING_CUBE_DIR,
    dont_use_existing_cube=False,
):
    base_dir = os.path.dirname(os.path.realpath(__file__))

    update_period_start_date = nd.next_day(
        date, offset=-int(days_update_period), scale="daily"
    )

    job = job.env(
        templates={
            "tvandroid_sessions_dir": tvandroid_sessions_dir,
            "watching_cube_dir": watching_cube_dir,
            "strm_dir": strm_dir,
        },
    )

    content_info = (
        job.table(CONTENT_INFO_PATH)
        .label("content_info")
        .filter(qf.defined("id"))
        .project(ne.all(exclude=["real_content_id", "id"]), content_id="id")
    )

    tvandroid_sessions = job.table(
        TVANDROID_SESSIONS_PATH.format(
            start_date=update_period_start_date, end_date=date
        )
    ).label("tvandroid_sessions")

    device_info = tvandroid_sessions.project(
        "device_id", "quasar_device_id"
    ).unique("device_id")
    has_plus_gift_info = (
        tvandroid_sessions.project(
            "device_id", "has_plus", "active_tv_gift", fielddate="event_date"
        )
        .groupby("fielddate", "device_id")
        .aggregate(
            has_plus=na.max("has_plus"),
            active_tv_gift=na.max("active_tv_gift"),
        )
    )

    watching_tvandroid = (
        tvandroid_sessions.filter(
            nf.equals("is_factory_ip", False),
            qf.one_of(
                "event_name",
                [
                    "webview_player_heartbeat",
                    "tv_player_heartbeat",
                    "youtube_player_heartbeat",
                ],
            ),
        )
        .project(
            "device_id",
            "session_id",
            "manufacturer",
            "model",
            "app_version",
            "diagonal",
            "resolution",
            "is_logged_in",
            "board",
            "build",
            "firmware_version",
            "platform",
            "puid",
            "has_plus",
            "active_tv_gift",
            "tandem_connection_state",
            "tandem_device_id",
            test_buckets="raw_buckets",
            fielddate="event_date",
            is_new_app_version=ne.custom(check_new_app_version, "app_version")
            .with_type(qt.Bool)
            .hide(),
            view_source=ne.custom(
                get_appmetrica_view_type,
                "event_name",
                "app_version",
                "event_value",
                "is_new_app_version",
            ).with_type(qt.Optional[qt.String]),
            clid1=ne.custom(
                lambda x: get_dict_path(x, ["clid1"], convert_type=str), "clids"
            ).with_type(qt.Optional[qt.String]),
            clid100010=ne.custom(
                lambda x: get_dict_path(x, ["clid100010"], convert_type=str),
                "clids",
            ).with_type(qt.Optional[qt.String]),
            channel_name=ne.custom(
                lambda x: get_dict_path(x, ["channel_name"]), "event_value"
            ).with_type(qt.Optional[qt.String]),
            channel_id=ne.custom(
                lambda x: str(get_dict_path(x, ["channel_id"])), "event_value"
            ).with_type(qt.Optional[qt.String]),
            url=ne.custom(
                lambda x: get_dict_path(x, ["url"]), "event_value"
            ).with_type(qt.Optional[qt.String]),
            channel_type=ne.custom(
                lambda x: get_dict_path(x, ["channel_type"]), "event_value"
            ).with_type(qt.Optional[qt.String]),
            period=ne.custom(
                lambda x: get_dict_path(x, ["period"]), "event_value"
            ).with_type(qt.Optional[qt.Int64]),
            player_session_id=ne.custom(
                get_player_session_id,
                "event_value",
                "view_source",
                "session_id",
                "url",
            ).with_type(qt.Optional[qt.String]),
        )
        .filter(
            nf.not_(nf.equals("view_source", "metrika_yaefir")),
        )
        .groupby(*APPMETRICA_WATCHING_LOG_FIELDS)
        .aggregate(view_time=na.sum("period"))
    )

    strm = (
        job.table(
            STRM_PATH.format(start_date=update_period_start_date, end_date=date)
        )
        .label("strm")
        .project(
            "device_id",
            "session_id",
            "manufacturer",
            "model",
            "clid1",
            "clid100010",
            "app_version",
            "board",
            "build",
            "firmware_version",
            "platform",
            "diagonal",
            "resolution",
            "fielddate",
            "monetization_model",
            "view_time",
            "channel_type",
            "content_type",
            "view_type",
            "test_buckets",
            "carousel_name",
            "carousel_position",
            "content_card_position",
            "tv_screen",
            "tv_parent_screen",
            "puid",
            "parent_id",
            "tandem_connection_state",
            "tandem_device_id",
            content_id="video_content_id",
            channel_name="channel",
            player_session_id="vsid",
            view_source=ne.custom(
                get_strm_view_type,
                "channel_type",
                "content_type",
                "view_type",
                "app_version",
            ).with_type(qt.Optional[qt.String]),
            is_logged_in=ne.custom(get_login_status, "puid").with_type(
                qt.Optional[qt.Bool]
            ),
        )
        .filter(
            nf.not_(nf.equals("view_source", "old_app_ya_efir_strm")),
        )
        .groupby(*STRM_WATCHING_LOG_FIELDS)
        .aggregate(view_time=na.sum("view_time"))
        .join(
            has_plus_gift_info,
            type="left",
            by=["fielddate", "device_id"],
            assume_unique_right=True,
        )
    )

    watching_cube_current = (
        job.concat(watching_tvandroid, strm)
        .project(
            ne.all(),
            is_recommendation_watching=ne.custom(
                check_recommendation_watching,
                "view_source",
                "tv_screen",
                "tv_parent_screen",
            ).with_type(qt.Optional[qt.Bool]),
        )
        .join(
            content_info, by="content_id", type="left", assume_unique_right=True
        )
        .join(
            device_info, by="device_id", type="left", assume_unique_right=True
        )
    )

    if dont_use_existing_cube:
        watching_cube = watching_cube_current
    else:
        watching_cube_past = (
            job.table(os.path.join(WATCHING_CUBE_DIR, "last"))
            .label("watching_cube_old")
            .filter(qf.compare("fielddate", "<", update_period_start_date))
        )

        watching_cube_future = (
            job.table(os.path.join(WATCHING_CUBE_DIR, "last"))
            .label("watching_cube_old")
            .filter(qf.compare("fielddate", ">", date))
        )

        watching_cube = job.concat(
            watching_cube_past, watching_cube_current, watching_cube_future
        )

    (
        watching_cube.sort(
            "fielddate", "manufacturer", "model", "app_version", "device_id"
        )
        .label("output")
        .put("$watching_cube_dir/{}".format(date), ttl=timedelta(7))
    )

    return job


@set_last_table_on_finish("date", "watching_cube_dir", WATCHING_CUBE_DIR)
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
