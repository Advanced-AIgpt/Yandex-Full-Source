#!/usr/bin/env python
# -*- coding: utf-8 -*-

from nile.api.v1 import (
    extractors as ne,
    files as nfl,
)

from qb2.api.v1 import extractors as qe, typing as qt, filters as qf, resources as qr

from utils.nirvana.op_caller import call_as_operation
from utils.yt.dep_manager import hahn_with_deps

from utils.common import get_dict_path

from utils.tv.app_version import (
    parse_app_version,
    get_resolution,
    get_diagonal,
)

from strm_utils import (
    get_channel_type,
    get_license,
    get_tandem_state,
)


STRM_CUBE_PATH = "//home/smarttv/logs/strm_micro_cube/1d/{date}"
CONTENT_INFO_PATH = "//home/sda/cubes/tv/content_info/last"
LOG_OUTPUT_PATH = "//home/smarttv/logs/strm/1d"


def make_job(job, date, strm_cube_path=STRM_CUBE_PATH, log_output_path=LOG_OUTPUT_PATH):
    strm_cube_path = strm_cube_path.format(date=date)

    content_info = (
        job.table(CONTENT_INFO_PATH)
        .label("content_info")
        .filter(qf.defined("id"))
        .project(
            ne.all(exclude=["real_content_id", "id", "content_availablity"]),
            video_content_id="id",
        )
    )

    job.table(strm_cube_path).qb2(
        log="generic-tskv-log",
        files=[
            nfl.StatboxDict("tvandroid_drop_ip.yaml"),
            nfl.StatboxDict("tvandroid_model_specifications.yaml"),
        ],
        fields=[
            qe.log_fields(
                "reqid",
                "vsid",
                "user_id",
                "yandexuid",
                "puid",
                "device_id",
                "fielddate",
                "timestamp",
                "user_agent",
                "os_family",
                "browser_name",
                "browser_version",
                "region",
                "view_type",
                "view_time",
                "video_content_id",
                "channel_id",
                "channel",
                "player_events",
                "errors",
                "test_buckets",
                "add_info",
                "license",
                "ref_from",
            ),
        ],
        filters=[
            qf.one_of("ref_from", ["tvandroid", "module2"]),
            qf.equals("fielddate", date),
        ],
    ).label(
        "strm"
    ).project(
        ne.all(exclude=["license", "ref_from"]),
        session_id=ne.custom(
            lambda x: get_dict_path(x, ["tvandroid_data", "session_id"]), "add_info"
        ).with_type(qt.Optional[qt.String]),
        clid1=ne.custom(
            lambda x: get_dict_path(x, ["tvandroid_data", "clid1"]), "add_info"
        ).with_type(qt.Optional[qt.String]),
        clid100010=ne.custom(
            lambda x: get_dict_path(x, ["tvandroid_data", "clid100010"]), "add_info"
        ).with_type(qt.Optional[qt.String]),
        model=ne.custom(
            lambda x: get_dict_path(x, ["tvandroid_data", "model"]), "add_info"
        ).with_type(qt.Optional[qt.String]),
        manufacturer=ne.custom(
            lambda x: get_dict_path(x, ["tvandroid_data", "manufacturer"]), "add_info"
        ).with_type(qt.Optional[qt.String]),
        content_type=ne.custom(
            lambda x: get_dict_path(x, ["tvandroid_data", "content_type"]), "add_info"
        ).with_type(qt.Optional[qt.String]),
        carousel_name=ne.custom(
            lambda x: get_dict_path(
                x, ["tvandroid_data", "startup_place", "carousel_name"]
            ),
            "add_info",
        ).with_type(qt.Optional[qt.String]),
        parent_id=ne.custom(
            lambda x: get_dict_path(
                x, ["tvandroid_data", "startup_place", "parent_id"]
            ),
            "add_info",
        ).with_type(qt.Optional[qt.String]),
        carousel_position=ne.custom(
            lambda x: get_dict_path(
                x, ["tvandroid_data", "startup_place", "carousel_position"]
            ),
            "add_info",
        ).with_type(qt.Optional[qt.Int64]),
        content_card_position=ne.custom(
            lambda x: get_dict_path(
                x, ["tvandroid_data", "startup_place", "content_card_position"]
            ),
            "add_info",
        ).with_type(qt.Optional[qt.Int64]),
        tv_screen=ne.custom(
            lambda x: get_dict_path(x, ["tvandroid_data", "startup_place", "from"]),
            "add_info",
        ).with_type(qt.Optional[qt.String]),
        tv_parent_screen=ne.custom(
            lambda x: get_dict_path(
                x, ["tvandroid_data", "startup_place", "parent_from"]
            ),
            "add_info",
        ).with_type(qt.Optional[qt.String]),
        app_version=ne.custom(
            lambda x: get_dict_path(x, ["tvandroid_data", "app_version"]), "add_info"
        ).with_type(qt.Optional[qt.String]),
        tandem_device_id=ne.custom(
            lambda x: get_dict_path(x, ["tvandroid_data", "tandem_device_id"]) or "", "add_info"
        ).with_type(qt.String),
        tandem_connection_state=ne.custom(
            lambda x: get_tandem_state(get_dict_path(x, ["tvandroid_data", "tandem_connection_state"], "")), "add_info"
        ).with_type(qt.String),
        parsed_app_version=ne.custom(parse_app_version, "app_version")
        .with_type(qt.Yson)
        .hide(),
        board=ne.custom(
            lambda x: x.get("board"),
            "parsed_app_version",
        ).with_type(qt.Optional[qt.String]),
        platform=ne.custom(
            lambda x: x.get("platform"),
            "parsed_app_version",
        ).with_type(qt.Optional[qt.String]),
        firmware_version=ne.custom(
            lambda x: x.get("firmware_version"),
            "parsed_app_version",
        ).with_type(qt.Optional[qt.String]),
        build=ne.custom(
            lambda x: x.get("build"),
            "parsed_app_version",
        ).with_type(qt.Optional[qt.String]),
        channel_type=ne.custom(get_channel_type, "channel").with_type(
            qt.Optional[qt.String]
        ),
        monetization_model=ne.custom(get_license, "license", "view_type").with_type(
            qt.Optional[qt.String]
        ),
        diagonal=ne.custom(
            get_diagonal,
            qr.yaml("tvandroid_model_specifications.yaml"),
            "manufacturer",
            "model",
        ).with_type(qt.Optional[qt.Float]),
        resolution=ne.custom(
            get_resolution,
            qr.yaml("tvandroid_model_specifications.yaml"),
            "manufacturer",
            "model",
        ).with_type(qt.Optional[qt.String]),
    ).join(
        content_info, by="video_content_id", type="left", assume_unique_right=True
    ).label(
        "output"
    ).put(
        log_output_path + "/" + date
    )

    return job


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
