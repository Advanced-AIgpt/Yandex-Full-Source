"""
This is a module for timespent metric calculation
It is now computed from one line
This code yields precompute - later will be used in timespent calculation
"""
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
import copy

from cofe.projects.alice.timespent.mappings import (
    TLT_TVT_SCENARIOS, APPS_WITH_HEARTBEATS, IGNORED_SCENARIOS
)
from cofe.projects.alice.timespent.metrics import (
    MILLISECONDS_IN_1_SECOND
)

from alice.analytics.operations.timespent.mappings import get_scenario, ifnull
from alice.analytics.operations.timespent.support_timespent_functions import TimespentRecord, get_table_range
from alice.analytics.operations.timespent.device_mappings import map_device_to_app, map_device_appmetic_to_device_expboxes

from alice.analytics.operations.dialog.sessions.intent_scenario_mapping import MUSIC_GENRES, SKILL_ID_MAPPING

HANDMADE_MAPPED_SCENARIOS = set(MUSIC_GENRES.values()).union(set(SKILL_ID_MAPPING.values()))


@with_hints(output_schema=dict(
    fielddate=typing.String,
    device_id=typing.String,
    user_id=typing.String,
    uuid=typing.String,
    puid=typing.String,
    country_id=typing.Optional[typing.Int64],
    app=typing.String,
    device=typing.String,
    first_day=typing.String,
    child_confidence=typing.Optional[typing.Float],
    generic_scenario=typing.String,
    parent_scenario=typing.Optional[typing.String],
    is_tv_plugged_in=typing.Bool,
    req_id=typing.String,
    parent_req_id=typing.Optional[typing.String],
    music_genre=typing.Optional[typing.String],
    server_time_ms=typing.Optional[typing.Int64],
    alice_speech_end_ms=typing.Optional[typing.Int64],
    timestamp=typing.Optional[typing.Int64],
    input_type=typing.Optional[typing.String],
    voice_text=typing.Optional[typing.String],
    scenario=typing.Optional[typing.String],
    reply=typing.Optional[typing.String],
    query=typing.Optional[typing.String],
    client_time=typing.Optional[typing.Int64],
    expboxes=typing.Optional[typing.String],
    testids=typing.Optional[typing.Yson],
))
def project_features_expboxes(records):
    for record in records:
        scenario = record['generic_scenario']
        if record.get('parent_scenario') and not (record['generic_scenario'] in HANDMADE_MAPPED_SCENARIOS):
            scenario = record.get('parent_scenario')
        yield Record(
            fielddate=record['fielddate'],
            device_id=ifnull(record.get('device_id', ''), ""),
            user_id=ifnull(record.get('user_id', ''), ""),
            uuid=record['uuid'],
            puid=ifnull(record.get('puid', ''), ""),
            country_id=record.get("country_id"),
            app=record['app'],
            device=record.get('device', ''),
            first_day=record['first_day'],
            child_confidence=record.get('child_confidence'),
            generic_scenario=record.get('generic_scenario'),
            parent_scenario=record.get("parent_scenario"),
            is_tv_plugged_in=ifnull(record.get('is_tv_plugged_in', False), False),
            req_id=record['req_id'],
            parent_req_id=record.get('parent_req_id'),
            music_genre=record.get('music_genre'),
            server_time_ms=record['server_time_ms'],
            alice_speech_end_ms=record['alice_speech_end_ms'],
            timestamp=record['server_time_ms'],
            input_type=record.get("input_type"),
            voice_text=record.get("voice_text"),
            scenario=get_scenario(scenario, record.get('music_genre')),
            reply=record.get("reply"),
            query=record.get("query"),
            client_time=record.get("client_time"),
            expboxes=record.get("expboxes"),
            testids=record.get("testids")
        )


@with_hints(output_schema=dict(
    fielddate=str,
    device_id=str,
    user_id=str,
    uuid=str,
    puid=str,
    country_id=typing.Optional[typing.Int64],
    app=str,
    device=str,
    first_day=str,
    child_confidence=typing.Optional[typing.Float],
    scenario=str,
    generic_scenario=str,
    parent_req_id=typing.Optional[typing.String],
    is_tv_plugged_in=typing.Optional[typing.Bool],
    req_id=str,
    parent_timespent_req_id=str,
    tlt_tvt=int,
    tts=int,
    tts_without_time_to_think=int,
    timestamp=int,
    expboxes=typing.Optional[typing.String],
    testids=typing.Optional[typing.Yson],
))
def extract_features(groups):
    for key_record, records in groups:
        baseRecord = TimespentRecord(fielddate=key_record['fielddate'], uuid=key_record['uuid'],
                                     app=key_record['app'], first_day=key_record['first_day'])

        running_sessions = {
            "main": copy.copy(baseRecord),
            "tts": copy.copy(baseRecord)
        }

        for record in records:
            # skip pressing buttons (there is instant next query with filled generic_scenario)
            # skip sound_commands and player_commands so they don't split sessions of other scenarios
            is_ignored_query = (record.get('scenario') in IGNORED_SCENARIOS and record.get("event_name") is None) \
                or (record.get("input_type") == "tech" and record.get("voice_text") is None)
            if is_ignored_query:
                continue
            current_timespent_record = copy.copy(baseRecord)
            current_timespent_record.set_keys_with_record(record)
            work_session = "tts"
            if (current_timespent_record.scenario in TLT_TVT_SCENARIOS or current_timespent_record.is_heartbeat) and current_timespent_record.app in APPS_WITH_HEARTBEATS:
                work_session = "main"
            if work_session == "main" and (not current_timespent_record.is_heartbeat):
                yield_record = running_sessions["tts"].close_session()
                if yield_record is not None:
                    yield yield_record
            if work_session == "tts" and (not running_sessions["main"].is_heartbeat or (current_timespent_record.scenario == "stop" and not current_timespent_record.is_heartbeat)):
                yield_record = running_sessions["main"].close_session(current_timespent_record.timestamp)
                if yield_record is not None:
                    yield yield_record

            yield_record = running_sessions[work_session].update_session(current_timespent_record)
            if yield_record is not None:
                yield yield_record

        for key in running_sessions:
            yield_record = running_sessions[key].close_session()
            if yield_record is not None:
                yield yield_record


@cli.statinfra_job
def make_job(job, nirvana, options):
    job = job.env()
    # yql backend can't project missing columns

    logs_expboxes_two_days = (
        job
        .table("$expboxes_path/" + get_table_range(options.dates[0], day_range=-1))
        .map(project_features_expboxes)
    )

    logs_expboxes = (
        logs_expboxes_two_days
        .filter(nf.custom(lambda fielddate: fielddate == options.dates[0], "fielddate"))
    )

    users = (
        logs_expboxes_two_days
        .filter(nf.custom(lambda x: x in APPS_WITH_HEARTBEATS, 'app'))
        .groupby('uuid')
        # join of heartbeats with expboxes by req_id misses a chunk of data
        .aggregate(app=na.any('app'), first_day=na.any('first_day'), device=na.any('device'),
                   user_id=na.any('user_id'), puid=na.any('puid'), country_id=na.any('country_id'))
    )

    # yql backend can't project missing columns
    requests = (
        logs_expboxes_two_days
        .filter(nf.custom(lambda x: x in APPS_WITH_HEARTBEATS, 'app'))
        .project("req_id", "child_confidence", "scenario", "generic_scenario", "is_tv_plugged_in", "music_genre", "expboxes", "testids")
    )

    logs_heartbeats = (
        job
        .table('$heartbeats_path/@dates')
        .project(ne.all(), parent_or_req_id=ne.custom(lambda req_id, parent_req_id:
                                                              parent_req_id if parent_req_id else req_id, 'req_id', 'parent_req_id').with_type(str))
        .join(users, by='uuid', type='left', assume_unique_right=True)
        .join(requests.project(ne.all(exclude="req_id"), parent_or_req_id="req_id"), by='parent_or_req_id', type='left', assume_unique_right=True)
        .project('send_timestamp', 'uuid', 'device_id', 'user_id', 'puid', 'event_name', 'country_id',
                 'first_day', 'child_confidence', 'req_id', 'is_tv_plugged_in',  'generic_scenario', 'music_genre', "parent_req_id", "expboxes", "testids",
                 app=ne.custom(lambda app, model: map_device_to_app(model) if map_device_to_app(model) else app, "app", "model").with_type(typing.Optional[typing.String]),
                 device=ne.custom(lambda device, model: map_device_appmetic_to_device_expboxes(model)
                                  if map_device_appmetic_to_device_expboxes(model) else device, "device", "model").with_type(typing.Optional[typing.String]),
                 fielddate='dt', scenario=ne.custom(lambda scenario, music_genre, event_name: get_scenario(scenario, music_genre, event_name), 'scenario', 'music_genre', 'event_name').with_type(str),
                 timestamp=ne.custom(lambda send_timestamp: int(send_timestamp*MILLISECONDS_IN_1_SECOND), "send_timestamp").with_type(int))
    )

    (
        logs_heartbeats
        .concat(logs_expboxes)
        .groupby('fielddate', 'app', 'uuid', 'first_day').sort('timestamp')
        .reduce(extract_features)
        .sort('uuid', 'timestamp')
        .put("$precompute_timespent/@dates")
    )

    return job


if __name__ == '__main__':
    cli.run()
