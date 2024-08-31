"""This is a module for timespent metric calculation
should have same logic as arcadia/quality/ab_testing/cofe/projects/alice/timespent
"""
from nile.api.v1 import (
    cli,
    filters as nf,
    extractors as ne,
    with_hints,
    Record
)
from qb2.api.v1 import (
    typing
)

from alice.analytics.operations.timespent.mappings import get_scenario_from_view_source, NOT_IN_TLT_TVT_MUSIC_SCENARIO, NOT_JOINED_MUSIC_SCENARIO
from cofe.projects.alice.timespent.metrics import (
    MILLISECONDS_IN_1_SECOND
)
from cofe.projects.alice.timespent.mappings import TLT_TVT_SCENARIOS
from alice.analytics.operations.timespent.support_timespent_functions import aggregate_timespent, is_valid_record
TLT_TVT_SCENARIOS.add(NOT_JOINED_MUSIC_SCENARIO)


@with_hints(output_schema=dict(
    fielddate=typing.String,
    child_confidence=typing.Optional[typing.Float],
    app=typing.Optional[typing.String],
    first_day=typing.Optional[typing.String],
    is_tv_plugged_in=typing.Optional[typing.Bool],
    scenario=typing.Optional[typing.String],
    tlt_tvt=typing.Optional[typing.Int64],
    device=typing.Optional[typing.String],
    uuid=typing.Optional[typing.String]
))
def map_tv_table(records):
    for record in records:
        scenario = get_scenario_from_view_source(record.get("view_source"))
        device = record.get("platform")

        yield Record(
            fielddate=record.get('fielddate'),
            child_confidence=None,
            app="tv",
            first_day="",
            is_tv_plugged_in=True,
            scenario=scenario,
            tlt_tvt=int((record.get("view_time") if record.get("view_time") else 0)*MILLISECONDS_IN_1_SECOND),
            device=device,
            uuid=""
        )


@cli.statinfra_job
def make_job(job, nirvana, options):
    job = job.env()

    # by connection sort order

    devices = job.table(nirvana.input_tables[0])
    users = job.table(nirvana.input_tables[1])
    weekly_devices = job.table(nirvana.input_tables[2])
    tv_devices = job.table(nirvana.input_tables[3])

    # yql backend can't project missing columns
    tv = (
        job
        .table("$tv_path")
        .filter(nf.custom(lambda date: date == options.dates[0], "fielddate"))
        .map(map_tv_table)
    )

    logs = (
        job
        .table("$precompute_timespent/@dates")
        .filter(nf.custom(is_valid_record, 'app', 'scenario', 'tlt_tvt', 'device'))
        .project(ne.all(exclude=["scenario"]), scenario=ne.custom(lambda scenario, tlt_tvt: scenario if
                                                                  (tlt_tvt == 0 or (tlt_tvt > 0 and scenario in TLT_TVT_SCENARIOS))
                                                                  else NOT_IN_TLT_TVT_MUSIC_SCENARIO, 'scenario', 'tlt_tvt').with_type(typing.Optional[typing.String]))
        .concat(tv)
    )

    timespent = aggregate_timespent(logs)

    (
        timespent
        .join(devices, type='left', by=['fielddate', 'app', 'cohort', 'age_category', 'is_tv_plugged_in', 'device'],
              assume_unique_right=True, assume_small_right=True)
        .join(weekly_devices.project(ne.all(exclude=['devices']), weekly_devices='devices'), type='left', by=['fielddate', 'app', 'cohort', 'age_category', 'is_tv_plugged_in', 'device'],
              assume_unique_right=True, assume_small_right=True)
        .join(users, type='left', by=['fielddate', 'app', 'cohort', 'age_category', 'is_tv_plugged_in', 'device'],
              assume_unique_right=True, assume_small_right=True)
        .join(tv_devices, type='left',
              by=['fielddate', 'app', 'cohort', 'age_category', 'is_tv_plugged_in', 'device'],
              assume_unique_right=True, assume_small_right=True)
        .put('$total_timespent_path/@dates')
    )

    return job


if __name__ == '__main__':
    cli.run()
