"""
Module for calculating second week timespent
"""
from nile.api.v1 import (
    cli,
    aggregators as na,
    filters as nf,
    extractors as ne,
    with_hints,
    Record
)
import itertools

from alice.analytics.operations.timespent.mappings import (
    get_age_category,
    ifnull,
    APPS_WITH_DEVICE_MODELS
)

from qb2.api.v1 import (
    typing
)

from cofe.projects.alice.timespent.metrics import (
    MILLISECONDS_IN_1_MINUTE, SECONDS_IN_1_MINUTE
)

from alice.analytics.operations.timespent.support_timespent_functions import map_timespent_aggregated_records, get_table_range, add_time_delta
from alice.analytics.operations.timespent.device_mappings import map_device_appmetic_to_device_expboxes, map_device_to_app
SPEAKERS_APPS = {"quasar", "small_smart_speakers"}


@with_hints(output_schema=dict(
    device_id=typing.String,
    age_category=typing.Optional[typing.String],
    app=typing.Optional[typing.String],
    device=typing.Optional[typing.String],
    first_day=typing.Optional[typing.String],
    is_tv_plugged_in=typing.Optional[typing.String],
))
def map_activated_devices(records):
    for record in records:
        device_list = ["_total_"]
        age_category_list = ["_total_"]
        app = record.get("app")
        age_category = "child" if record.get("has_child_requests") else ""
        is_tv_plugged_in = "true" if record.get("is_tv_plugged_in") else "false"

        if app in APPS_WITH_DEVICE_MODELS and record.get('device'):
            device_list.append(record.get('device'))
        if age_category == "child":
            age_category_list.append(age_category)
        for tmp_age_category, tmp_app, tmp_device, tmp_is_tv_plugged_in in itertools.product(
            age_category_list, (app, "_total_"), device_list, (is_tv_plugged_in, "_total_")
        ):
            yield Record(
                device_id=ifnull(record.get('device_id', ''), ""),
                age_category=tmp_age_category,
                device=tmp_device,
                app=tmp_app,
                is_tv_plugged_in=tmp_is_tv_plugged_in,
            )


@cli.statinfra_job
def make_job(job, nirvana, options):
    job = job.env()

    activation_date = options.dates[0]
    # yql backend can't project missing columns

    devices_activated = (
        job
        .table("$retention_activated")
        .filter(nf.custom(lambda date: date == activation_date, "activation_date"))
        .groupby("user_id")
        .aggregate(has_child_requests=na.max("has_child_requests"), hdmi_plugged=na.max("hdmi_plugged"), device_type=na.any("device_type"))
        .project("has_child_requests",
                 device_id="user_id",
                 is_tv_plugged_in=ne.custom(lambda hdmi_plugged: True if hdmi_plugged else False).with_type(bool),
                 app=ne.custom(map_device_to_app, "device_type").with_type(typing.Optional[typing.String]),
                 device=ne.custom(map_device_appmetic_to_device_expboxes, "device_type").with_type(typing.Optional[typing.String])
                 )
        .filter(nf.custom(lambda app: app in SPEAKERS_APPS, "app"))
    )

    devices_activated_sum = (
        devices_activated
        .map(map_activated_devices)
        .groupby("app", "device", "age_category", "is_tv_plugged_in")
        .aggregate(activated_devices=na.count_distinct("device_id"))
    )

    timespent = (
        job
        .table("$precompute_timespent/" + get_table_range(add_time_delta(activation_date, 7), 6))
        .filter(nf.custom(lambda app: app in SPEAKERS_APPS, "app"))
        .project("device", "app", "scenario", "tlt_tvt", "tts", "device_id",
                 age_category=ne.custom(get_age_category, "child_confidence").with_type(str),
                 cohort=ne.const(""), fielddate=ne.const("_total_"))
        .join(devices_activated.project("device_id", "is_tv_plugged_in"), type="inner", by="device_id",
              assume_unique_right=True, assume_small_right=True)
        .groupby('fielddate', 'cohort', 'app', 'scenario', 'age_category', 'is_tv_plugged_in', "device")
        .aggregate(tlt_tvt=na.sum('tlt_tvt'),
                   tts=na.sum('tts'))
        .map(map_timespent_aggregated_records)
        .groupby('fielddate', 'cohort', 'app', 'scenario', 'age_category', 'is_tv_plugged_in', 'device')
        .aggregate(tlt_tvt_s=na.sum('tlt_tvt_s'),
                   timespent_ms=na.sum('timespent_ms'))
        .project('fielddate', 'app', 'scenario', 'age_category',
                 'timespent_ms', 'tlt_tvt_s', 'is_tv_plugged_in', 'device',
                 total_timespent_m=ne.custom(
                     lambda a, b: ifnull(a, 0) / MILLISECONDS_IN_1_MINUTE + ifnull(b, 0) / SECONDS_IN_1_MINUTE,
                     'timespent_ms', 'tlt_tvt_s').with_type(float))

    )

    (
        timespent
        .join(devices_activated_sum, type='left', by=["app", "device", "age_category", "is_tv_plugged_in"],
              assume_unique_right=True, assume_small_right=True)
        .project(ne.all(exclude=["fielddate"]), fielddate=ne.const(activation_date))
        .put('$second_week_timespent_path/@dates')
    )

    return job


if __name__ == '__main__':
    cli.run()
