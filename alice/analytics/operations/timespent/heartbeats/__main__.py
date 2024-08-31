# encoding: utf-8
"""This is a module for heartbeats tlt metric calculation
should have same logic as arcadia/quality/ab_testing/cofe/projects/alice/heartbeats
"""

from collections import namedtuple
from nile.api.v1 import (
    cli,
    aggregators as na,
    filters as nf,
    extractors as ne,
    with_hints,
    Record
)
from qb2.api.v1 import (
    typing
)
import itertools

from cofe.projects.alice.timespent.mappings import (
    TLT_TVT_SCENARIOS, APPS_WITH_HEARTBEATS
)
from alice.analytics.operations.timespent.mappings import get_scenario, get_age_category, APPS_WITH_DEVICE_MODELS
from alice.analytics.operations.timespent.device_mappings import filter_device_models_in_apps

HEARTBEAT_EPS = 35.


@with_hints(output_schema=dict(
    req_id=str,
    server_time_ms=int,
    generic_scenario=str,
    is_tv_plugged_in=typing.Optional[typing.Bool],
    music_genre=typing.Optional[typing.String],
    child_confidence=typing.Optional[typing.Float],
))
def project_features(records):
    for record in records:
        yield Record(
            req_id=record['req_id'],
            server_time_ms=record['server_time_ms'],
            generic_scenario=record['parent_scenario'] if record['parent_scenario'] else record['generic_scenario'],
            music_genre=record.get('music_genre'),
            child_confidence=record.get('child_confidence'),
            is_tv_plugged_in=record.get('is_tv_plugged_in')
        )


@with_hints(output_schema=dict(fielddate=str, uuid=str, scenario=str, app=str, tlt_tvt_s=int, age_category=str, cohort=str, is_tv_plugged_in=str, device=str))
def extract_features(groups):
    for key_record, records in groups:
        uuid = key_record['uuid']
        fielddate = key_record['dt']
        app = key_record['app']
        cohort = key_record['cohort']
        age_category = key_record['age_category']
        prev_req_time = None
        prev_scenario = None
        tlt_tvt = 0
        is_tv_plugged_in = False
        return_records = []
        RetRecord = namedtuple('RetRecord', [
            'fielddate',
            'uuid',
            'scenario',
            'app',
            'tlt_tvt',
            'age_category',
            'cohort',
            'device'
        ])
        for record in records:
            if record.get('is_tv_plugged_in'):
                is_tv_plugged_in = True
            req_time = record['send_timestamp']
            scenario = record['scenario']
            is_first_request = prev_req_time is None
            if not is_first_request and (scenario != prev_scenario or req_time - prev_req_time > HEARTBEAT_EPS):
                # changed scenario or exceeded timeout
                if req_time - prev_req_time <= HEARTBEAT_EPS:
                    tlt_tvt += req_time - prev_req_time
                if prev_scenario in TLT_TVT_SCENARIOS and app in APPS_WITH_HEARTBEATS:
                    device_list = ["_total_"]
                    if app in APPS_WITH_DEVICE_MODELS and record.get('device'):
                        device_list.append(record.get('device'))
                    for tmp_app, tmp_scenario, tmp_cohort, tmp_device in itertools.product(
                        (app, '_total_'), (prev_scenario, '_total_'),  (cohort, '_total_'), device_list
                    ):
                        return_records.append(
                            RetRecord(fielddate, uuid, tmp_scenario, tmp_app, tlt_tvt, age_category, tmp_cohort, tmp_device)
                        )
                tlt_tvt = 0
            elif not is_first_request:
                # hasn't exceeded timeout
                tlt_tvt += req_time - prev_req_time
            prev_req_time = req_time
            prev_scenario = scenario
        if prev_req_time is not None and prev_scenario in TLT_TVT_SCENARIOS and app in APPS_WITH_HEARTBEATS:
            device_list = ["_total_"]
            if app in APPS_WITH_DEVICE_MODELS and record.get('device'):
                device_list.append(record.get('device'))
            for tmp_app, tmp_scenario, tmp_cohort, tmp_device in itertools.product(
                (app, '_total_'), (scenario, '_total_'), (cohort, '_total_'), device_list
            ):
                return_records.append(
                    RetRecord(fielddate, uuid, tmp_scenario, tmp_app, tlt_tvt, age_category, tmp_cohort, tmp_device)
                )
        for record in return_records:
            for tv_status in (is_tv_plugged_in, "_total_"):
                yield Record(
                    fielddate=str(record.fielddate),
                    uuid=str(record.uuid),
                    scenario=str(record.scenario),
                    app=str(record.app),
                    tlt_tvt_s=int(record.tlt_tvt),
                    age_category=str(record.age_category),
                    cohort=str(record.cohort),
                    is_tv_plugged_in=str(tv_status).lower(),
                    device=str(record.device)
                )


@cli.statinfra_job
def make_job(job, nirvana, options):
    job = job.env()

    users = (
        job
        .table("$expboxes_path/@dates")
        .filter(nf.custom(lambda x: x in APPS_WITH_HEARTBEATS, 'app'))
        .groupby('uuid')
        # join of heartbeats with expboxes by req_id misses a chunk of data
        .aggregate(app=na.any('app'), cohort=na.any('cohort'), device=na.any('device'))
    )

    # yql backend can't project missing columns
    requests = (
        job
        .table("$expboxes_path/@dates")
        .filter(nf.custom(lambda x: x in APPS_WITH_HEARTBEATS, 'app'))
        .map(project_features)
    )

    logs = (
        job
        .table('$heartbeats_path/@dates')
        .project(ne.all(exclude=['parent_req_id', 'req_id']),
                 req_id=ne.custom(lambda req_id, parent_req_id:
                                  parent_req_id if parent_req_id else req_id, 'req_id', 'parent_req_id').with_type(str))
        .join(users, by='uuid', type='left', assume_unique_right=True)
        .join(requests, by='req_id', type='left', assume_unique_right=True)
        .filter(nf.custom(filter_device_models_in_apps, "device", "app"))
        .project('send_timestamp', 'dt', 'uuid', 'app', 'cohort', 'device',
                 is_tv_plugged_in=ne.custom(lambda x: True if x else False, 'is_tv_plugged_in').with_type(bool),
                 scenario=ne.custom(get_scenario, 'generic_scenario', 'music_genre', 'event_name').with_type(str),
                 age_category=ne.custom(get_age_category, 'child_confidence').with_type(str))
    )
    kids = (
        logs
        .filter(nf.equals('age_category', 'child'))
        .groupby('dt', 'uuid', 'cohort', 'app', 'age_category').sort('send_timestamp')
        .reduce(extract_features)
    )

    (
        logs
        .project(ne.all(), age_category=ne.const('_total_'))
        .groupby('dt', 'uuid', 'cohort', 'app', 'age_category').sort('send_timestamp')
        .reduce(extract_features)
        .concat(kids)
        .groupby('fielddate', 'cohort', 'app', 'scenario', 'age_category', 'is_tv_plugged_in', "device")
        .aggregate(tlt_tvt_s=na.sum('tlt_tvt_s'))
        .put('$tmp_path/@dates')
    )

    return job


if __name__ == '__main__':
    cli.run()
