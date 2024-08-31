# encoding: utf-8

from nile.api.v1 import (
    aggregators as na,
    extractors as ne,
    with_hints,
    Record
)
from qb2.api.v1 import (
    typing
)
import itertools
import datetime as dt

from cofe.projects.alice.timespent.metrics import (
    MILLISECONDS_IN_1_MINUTE, SECONDS_IN_1_MINUTE
)

from alice.analytics.operations.timespent.mappings import (
    get_age_category,
    ifnull
)

from alice.analytics.operations.timespent.device_mappings import VALID_DEVICES

from alice.analytics.operations.timespent.mappings import HEARTBEAT_EPS_MS
from alice.analytics.operations.timespent.device_mappings import filter_device_models_in_apps

from cofe.projects.alice.timespent.mappings import (
    TIME_TO_THINK_MS
)

get_week_ago = lambda date: (dt.datetime.strptime(date, "%Y-%m-%d") - dt.timedelta(days=7)).strftime("%Y-%m-%d")
get_week_table_range = lambda date : "{" + get_week_ago(date) + ".." + date + "}"


def add_time_delta(date, day_range=1):
    day_range_added = (dt.datetime.strptime(date, "%Y-%m-%d") + dt.timedelta(days=day_range)).strftime("%Y-%m-%d")
    return day_range_added


def get_table_range(date, day_range=1):
    day_range_ago = add_time_delta(date, day_range)
    if day_range < 0:
        return "{" + day_range_ago + ".." + date + "}"
    else:
        return "{" + date + ".." + day_range_ago + "}"


def is_valid_record(app, scenario, tlt_tvt, device):
    valid_record = ((scenario != "None" and app != "None") and app and scenario
                    and filter_device_models_in_apps(device, app) and (tlt_tvt == 0 or device in VALID_DEVICES or app == 'tv'))
    return valid_record


def get_start_of_the_week(date):
    try:
        date_obj = dt.datetime.strptime(date, "%Y-%m-%d")
        return (date_obj - dt.timedelta(days=date_obj.weekday())).strftime("%Y-%m-%d")
    except:
        return ""


class TimespentRecord:
    """
    Timespent Record is base class for dealing with user queries + heartbeats
    It accumulates information about user session
    Record can be renewed with next record - it processes one-line time-sorted record as they are processed in timespent metric
    """
    TimespentRecordKeys = {
        "device_id": "",
        "user_id": "",
        "uuid": "",
        "puid": "",
        "app": "",
        "fielddate": "",
        "country_id": 0,
        "child_confidence": None,
        "first_day": "",
        "scenario": "",
        "is_tv_plugged_in": False,
        "device": "",
        "req_id": "",
        "parent_timespent_req_id": "",
        "tts": 0,
        "tlt_tvt": 0,
        "tts_without_time_to_think": 0,
        "timestamp": 0,
        "timestamp_start": 0,
        "alice_speech_end_ms": 0,
        "is_heartbeat": False,
        "generic_scenario": "",
        "parent_req_id": "",
        "expboxes": "",
        "testids": None,
    }
    changingRecords = ["puid", "user_id", "device_id", "country_id", "child_confidence", "scenario", "is_tv_plugged_in",
                       "device", "req_id", "timestamp", "alice_speech_end_ms", "is_heartbeat", "generic_scenario", "parent_req_id", "expboxes", "testids"]

    def __init__(self, **kwargs):
        for key in self.TimespentRecordKeys:
            if key in kwargs:
                setattr(self, key, kwargs[key])
            else:
                setattr(self, key, self.TimespentRecordKeys[key])
        if set(kwargs) - set(self.TimespentRecordKeys):
            raise ValueError("Timespent record contains impossible parameters")

    def set_keys(self, **kwargs):
        for key in kwargs:
            if key in self.TimespentRecordKeys:
                setattr(self, key, kwargs[key])
            else:
                raise ValueError("Timespent update contains impossible parameters")

    def set_keys_with_record(self, record):
        for key in self.changingRecords:
            if key in record:
                setattr(self, key, record.get(key))
            else:
                setattr(self, key, self.TimespentRecordKeys[key])
        self.parent_timespent_req_id = self.req_id
        self.timestamp_start = self.timestamp
        if record.get("event_name") is not None:
            self.is_heartbeat = True
        else:
            self.is_heartbeat = False

    def update_record_with_next(self, next_timespent_record):
        for key in self.changingRecords:
            setattr(self, key, getattr(next_timespent_record, key))
        if self.alice_speech_end_ms is None or self.alice_speech_end_ms < self.timestamp:
            self.alice_speech_end_ms = self.timestamp
        self.tts_without_time_to_think = self.alice_speech_end_ms - self.timestamp
        self.tlt_tvt = 0
        self.tts = 0

    def close_session(self, close_timestamp=None):
        if self.tts_without_time_to_think > 0:
            self.tts += self.tts_without_time_to_think
        elif self.is_heartbeat and close_timestamp:
            if close_timestamp - self.timestamp < HEARTBEAT_EPS_MS:
                self.tlt_tvt += close_timestamp - self.timestamp
        return_record = self.get_record()
        for key in self.changingRecords:
            setattr(self, key, self.TimespentRecordKeys[key])
        self.timestamp_start = 0
        if self.alice_speech_end_ms is None or self.alice_speech_end_ms < self.timestamp:
            self.alice_speech_end_ms = self.timestamp
        self.tts_without_time_to_think = self.alice_speech_end_ms - self.timestamp
        self.tlt_tvt = 0
        self.tts = 0
        return return_record

    def update_session(self, next_timespent_record):
        '''
        main update timespent function, updates record and returns records to be yielded
        '''
        if self.scenario != next_timespent_record.scenario:
            # get record, change record, return record
            if self.tts_without_time_to_think > 0:
                self.tts += self.tts_without_time_to_think
            elif self.is_heartbeat:
                timeout_const = HEARTBEAT_EPS_MS
                timeout_exceeded = (next_timespent_record.timestamp - self.timestamp) > timeout_const
                if not timeout_exceeded:
                    self.tlt_tvt += next_timespent_record.timestamp - self.timestamp
            return_record = self.get_record()
            self.update_record_with_next(next_timespent_record)
            self.timestamp_start = self.timestamp
            self.parent_timespent_req_id = next_timespent_record.parent_timespent_req_id
            return return_record
        else:
            timeout_const = HEARTBEAT_EPS_MS if (self.is_heartbeat or next_timespent_record.is_heartbeat) else TIME_TO_THINK_MS
            if (self.is_heartbeat):
                timeout_exceeded = (next_timespent_record.timestamp - self.timestamp) > timeout_const
            else:
                timeout_exceeded = (next_timespent_record.timestamp - self.alice_speech_end_ms) > timeout_const
            if not timeout_exceeded:
                if self.is_heartbeat:
                    self.tlt_tvt += next_timespent_record.timestamp - self.timestamp
                else:
                    self.tts += next_timespent_record.timestamp - self.timestamp
            elif not self.is_heartbeat:
                self.tts += self.tts_without_time_to_think
            req_id_changed = self.req_id != next_timespent_record.req_id
            type_changed = self.is_heartbeat != next_timespent_record.is_heartbeat
            self.timestamp = next_timespent_record.timestamp
            if req_id_changed or type_changed or timeout_exceeded:
                return_record = self.get_record()
                self.update_record_with_next(next_timespent_record)
                self.timestamp_start = self.timestamp
                if timeout_exceeded:
                    self.parent_timespent_req_id = next_timespent_record.req_id
                return return_record

    def get_record(self):
        if self.uuid and self.scenario and self.app:
            return Record(
                fielddate=str(self.fielddate),
                device_id=str(self.device_id),
                user_id=str(self.user_id),
                uuid=str(self.uuid),
                puid=str(self.puid),
                country_id=self.country_id,
                app=str(self.app),
                device=str(self.device),
                first_day=str(self.first_day),
                child_confidence=self.child_confidence,
                scenario=str(self.scenario),
                generic_scenario=str(self.generic_scenario),
                parent_req_id=self.parent_req_id,
                is_tv_plugged_in=self.is_tv_plugged_in,
                req_id=str(self.req_id),
                parent_timespent_req_id=self.parent_timespent_req_id,
                tlt_tvt=int(self.tlt_tvt),
                tts=int(self.tts),
                tts_without_time_to_think=int(self.tts_without_time_to_think),
                timestamp=int(self.timestamp_start),
                expboxes=self.expboxes,
                testids=self.testids
            )


@with_hints(output_schema=dict(
    fielddate=typing.String,
    age_category=typing.Optional[typing.String],
    app=typing.Optional[typing.String],
    cohort=typing.Optional[typing.String],
    is_tv_plugged_in=typing.Optional[typing.String],
    scenario=typing.Optional[typing.String],
    tlt_tvt_s=typing.Optional[typing.Int64],
    timespent_ms=typing.Optional[typing.Int64],
    device=typing.Optional[typing.String]
))
def map_timespent_aggregated_records(records):
    '''
    Duplicator function for timespent records - produces both total and regular records
    '''
    for record in records:
        app = record.get("app")
        scenario = record.get("scenario")
        age_category = record.get("age_category")
        cohort = record.get("cohort")
        tlt_tvt = record.get("tlt_tvt") if record.get("tlt_tvt") else 0

        device_list = ["_total_"]
        age_category_list = ["_total_"]
        cohort_list = ["_total_"]

        if record.get('device') in VALID_DEVICES or app == "tv":
            device_list.append(record.get('device'))
        if age_category == "child":
            age_category_list.append(age_category)
        if cohort:
            cohort_list.append(cohort)

        for tmp_app, tmp_scenario, tmp_cohort, tmp_device, tmp_age_category, tmp_is_tv_plugged_in in itertools.product(
            (app, '_total_'), (scenario, '_total_'), cohort_list, device_list, age_category_list, (record.get("is_tv_plugged_in"), "_total_")
        ):
            yield Record(
                fielddate=record.get('fielddate'),
                age_category=tmp_age_category,
                app=tmp_app,
                cohort=tmp_cohort,
                is_tv_plugged_in=str(tmp_is_tv_plugged_in).lower(),
                scenario=tmp_scenario,
                tlt_tvt_s=tlt_tvt//1000 if tlt_tvt else record.get("tlt_tvt"),
                timespent_ms=record.get("tts"),
                device=tmp_device
            )


def aggregate_timespent(logs):
    '''
    input in the format of timespent_precompute tables
    takes any amount of filtered data and returns a table with timespent aggregated by columns:
    ['fielddate', 'cohort', 'app', 'scenario', 'age_category', 'is_tv_plugged_in', 'device']
    with totals
    '''
    logs = (
        logs
        .project("device", "app", "fielddate", "scenario", "uuid", "tlt_tvt", "tts",
                 is_tv_plugged_in=ne.custom(
                     lambda is_tv_plugged_in: is_tv_plugged_in if is_tv_plugged_in else False).with_type(bool),
                 cohort=ne.custom(get_start_of_the_week, "first_day").with_type(str),
                 age_category=ne.custom(get_age_category, "child_confidence").with_type(str))
    )

    tv_plugged_data = (
        logs
        .groupby('uuid', 'fielddate')
        .aggregate(is_tv_plugged_in=na.max('is_tv_plugged_in'))
    )

    aggregated_timespent = (
        logs
        .project(ne.all(exclude="is_tv_plugged_in"))
        .join(tv_plugged_data, type='left', by=["uuid", "fielddate"], assume_unique_right=True)
        .groupby('fielddate', 'cohort', 'app', 'scenario', 'age_category', 'is_tv_plugged_in', "device")
        .aggregate(tlt_tvt=na.sum('tlt_tvt'),
                   tts=na.sum('tts'))
        .map(map_timespent_aggregated_records)
        .groupby('fielddate', 'cohort', 'app', 'scenario', 'age_category', 'is_tv_plugged_in', 'device')
        .aggregate(tlt_tvt_s=na.sum('tlt_tvt_s'),
                   timespent_ms=na.sum('timespent_ms'))
        .project('fielddate', 'app', 'scenario', 'age_category', 'cohort',
                 'timespent_ms', 'tlt_tvt_s', 'is_tv_plugged_in', 'device',
                 total_timespent_m=ne.custom(
                     lambda a, b: ifnull(a, 0) / MILLISECONDS_IN_1_MINUTE + ifnull(b, 0) / SECONDS_IN_1_MINUTE,
                     'timespent_ms', 'tlt_tvt_s').with_type(float))
    )

    return aggregated_timespent
