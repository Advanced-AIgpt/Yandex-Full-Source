# -*-coding: utf8 -*-

import datetime
from os import path
import random
import time

from nile.api.v1 import (
    extractors as ne,
    filters as nf,
    Record,
    extended_schema,
    with_hints,
)
from qb2.api.v1 import (
    typing as qt
)

from utils.nirvana.op_caller import call_as_operation
from utils.yt.dep_manager import hahn_with_deps
import utils.yt.basket_common as common


APP_LIST = {
    "ru.yandex.searchplugin": "search_app_prod",
    "ru.yandex.mobile": "search_app_prod",
    "ru.yandex.searchplugin.beta": "search_app_beta",
    "ru.yandex.mobile.search": "browser_prod",
    "ru.yandex.mobile.search.ipad": "browser_prod",
    "winsearchbar": "stroka",
    "com.yandex.browser": "browser_prod",
    "com.yandex.browser.alpha": "browser_alpha",
    "com.yandex.browser.beta": "browser_beta",
    "ru.yandex.yandexnavi": "navigator",
    "ru.yandex.mobile.navigator": "navigator",
    "com.yandex.launcher": "launcher",
    "ru.yandex.quasar.services": "quasar",
    "yandex.auto": "auto",
    "ru.yandex.autolauncher": "auto",
    "ru.yandex.iosdk.elariwatch": "elariwatch",
    "aliced": "small_smart_speakers",
    "YaBro": "yabro_prod",
    "YaBro.beta": "yabro_beta",
    "ru.yandex.quasar.app": "quasar",
    "yandex.auto.old": "auto_old",
    "ru.yandex.mobile.music": "music_app_prod",
    "ru.yandex.music": "music_app_prod",
    "com.yandex.tv.alice": "tv",
    "ru.yandex.taximeter": "taximeter",
    "ru.yandex.yandexmaps": "yandexmaps_prod",
    "ru.yandex.traffic": "yandexmaps_prod",
    "ru.yandex.yandexmaps.debug": "yandexmaps_dev",
    "ru.yandex.yandexmaps.pr": "yandexmaps_dev",
    "ru.yandex.traffic.inhouse": "yandexmaps_dev",
    "ru.yandex.traffic.sandbox": "yandexmaps_dev",
    "com.yandex.alice": "alice_app",
    "ru.yandex.centaur": "centaur",
    "ru.yandex.sdg.taxi.inhouse": "sdc",
    "com.yandex.iot": "iot_app",
    "legatus": "legatus",
}


def random_part(i):
    return "".join(map(lambda x: random.choice("0123456789abcdef"), range(i)))


def generate_id(row_index):
    return "".join(["ffffffff-ffff-ffff-", random_part(4), "-", random_part(12)])


def remove_device_ids(device_state):
    if device_state and "device_id" in device_state:
        del device_state["device_id"]
    return device_state


@with_hints(output_schema=extended_schema(row_index=qt.Integer))
def get_row_index(records):
    for record in records:
        yield Record(record, row_index=records.row_index)


def get_app_preset_from_request(request):
    device_manufacturer = request["app_info"].get("device_manufacturer") or  ""
    device_model = request["app_info"].get("device_model") or ""
    device = " ".join([device_manufacturer, device_model])
    app = APP_LIST.get(request.get("app_info", {}).get("app_id"), "other")
    return common.get_app_preset_from_app(app, device)


def main(apps, input_table, output_table=None, pool=None, passed_columns=None, add_robot_logs=False):
    templates = {"job_root": "//tmp/robot-voice-qa"}
    cluster = hahn_with_deps(
        templates=templates,
        use_yql=False,
        include_utils=True,
        pool=pool,
    )
    io_option = {"table_writer": {"max_row_weight": 134217728}}
    yt_spec = {name + "job_io": io_option for name in ["", "partition_", "merge_", "map_", "reduce_", "sort_"]}
    job = cluster.job().env(yt_spec_defaults=yt_spec)

    output_table = output_table or "//tmp/robot-voice-qa/new_scenarious_basket_" + random_part(16)
    tmp_table = "//tmp/robot-voice-qa/vins_tmp_sample" + random_part(16)

    start_date = datetime.datetime.now() - datetime.timedelta(days=5)
    end_date = datetime.datetime.now() - datetime.timedelta(days=5)
    date_str = "{{{0:%Y-%m-%d}..{1:%Y-%m-%d}}}".format(start_date, end_date)

    projected_fields = ["text"]
    if passed_columns:
        projected_fields += passed_columns

    dialogs_sample = (
        job
        .table(path.join("//home/alice/wonder/dialogs", date_str), ignore_missing=True)
        .random(fraction=0.001)
    )

    if add_robot_logs:
        end_date = datetime.datetime.now() - datetime.timedelta(days=3)
        date_str = "{{{0:%Y-%m-%d}..{1:%Y-%m-%d}}}".format(start_date, end_date)

        dialogs_sample = job.concat(
            job.table(path.join("//home/alice/wonder/robot-dialogs", date_str), ignore_missing=True),
            dialogs_sample,
        )

    device_states_sample = (
        dialogs_sample
        .project(
            app_preset=ne.custom(get_app_preset_from_request, "request").with_type(qt.String),
            device_state=ne.custom(lambda request: remove_device_ids(request.get("device_state")), "request").with_type(qt.Json),
            location=ne.custom(
                lambda request, analytics_info: analytics_info.get("location") or request.get("location"),
                "request", "analytics_info"
            )
        )
        .filter(
            nf.custom(lambda app: app in set(apps), "app_preset"),
            nf.not_(nf.equals("device_state", {})),
        )
        .random(count=7000, memory_limit=16384)
        .put(tmp_table)
        .map(get_row_index, enable_row_index=True)
    )

    result = (
        job.table(input_table)
        .map(get_row_index, enable_row_index=True)
        .project(
            *projected_fields,
            voice_url="downloadUrl",
            row_index=ne.custom(lambda i: i % 10000, "row_index").with_type(qt.Integer)
        )
        .join(device_states_sample, by="row_index")
        .project(
            ne.all(exclude=["row_index"]),
            vins_intent=ne.const("").with_type(qt.String),
            fetcher_mode=ne.const("voice").with_type(qt.String),
            request_id=ne.custom(generate_id, "row_index").with_type(qt.String),
            session_sequence=ne.const(0).with_type(qt.Integer),
            reversed_session_sequence=ne.const(0).with_type(qt.Integer),
            toloka_intent=ne.const("other").with_type(qt.String),
            asr_options=ne.const({"allow_multi_utt": False}).with_type(qt.Json),
        )
        .project(ne.all(), session_id="request_id")
        .put(output_table)
    )

    with job.driver.transaction():
        job.run()

    return {"cluster": "hahn", "table": output_table}


if __name__ == "__main__":
    st = time.time()
    print("start at", time.ctime(st))
    call_as_operation(main)
    print("total elapsed {:2f} min".format((time.time() - st) / 60))
