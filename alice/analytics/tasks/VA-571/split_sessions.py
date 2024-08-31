from collections import OrderedDict
from functools import partial
from nile.api.v1 import Record
from utils.nirvana.op_caller import call_as_operation
from utils.yt.dep_manager import hahn_with_deps
from qb2.api.v1 import typing

SCHEMA = OrderedDict([
    ("alice_speech_end_ms", int),
    ("analytics_info", typing.Json),
    ("app", str),
    ("child_confidence", float),
    ("client_time", int),
    ("client_tz", str),
    ("cohort", str),
    ("country_id", int),
    ("device", str),
    ("device_id", str),
    ("do_not_use_user_logs", bool),
    ("expboxes", str),
    ("fielddate", str),
    ("first_day", str),
    ("generic_scenario", str),
    ("icookie", str),
    ("input_type", str),
    ("intent", str),
    ("is_exp_changed", bool),
    ("is_interrupted", bool),
    ("is_new", str),
    ("is_smart_home_user", bool),
    ("is_tv_plugged_in", bool),
    ("lang", str),
    ("location", typing.Json),
    ("mm_scenario", str),
    ("music_answer_type", str),
    ("music_genre", str),
    ("other", typing.Json),
    ("parent_req_id", str),
    ("parent_scenario", str),
    ("platform", str),
    ("puid", str),
    ("query", str),
    ("reply", str),
    ("req_id", str),
    ("request_act_type", str),
    ("screen", str),
    ("server_time_ms", int),
    ("session_id", str),
    ("session_id_old", str),
    ("session_sequence", int),
    ("skill_id", str),
    ("sound_level", float),
    ("sound_muted", bool),
    ("subscription", str),
    ("testids", typing.Json),
    ("trash_or_empty_request", bool),
    ("user_id", str),
    ("uuid", str),
    ("version", str),
    ("voice_text", str)
])


def split_sessions(groups, min_len, max_len):

    def split_by_big_delta(deltas, min_len, max_len, left=None, right=None):
        # ignore first and last min_len deltas
        if left is None:
            left = 0
        if right is None:
            right = len(deltas) - 1

        # len of subsession is one more than len of deltas
        if right + 1 - left + 1 <= max_len:
            return []

        # ignore first and last (min_len-1) deltas
        arg_max_ = left + min_len - 1
        max_ = deltas[arg_max_]
        i = left + min_len
        while i <= right - min_len + 1:
            if deltas[i][1] > max_[1]:
                arg_max_, max_ = i, deltas[i]
            i += 1

        res = ([arg_max_]
               + split_by_big_delta(deltas, min_len, max_len, left, arg_max_-1)
               + split_by_big_delta(deltas, min_len, max_len, arg_max_+1, right)
               )
        return res

    def skip_unnecessary_splits(splits, deltas_len, max_len):
        res = []
        start = 0
        for end in (splits + [deltas_len]):
            if end - start + 1 > max_len:
                # todo: add exception
                # first split must be less than max_len else we will fail here
                res.append(prev_end)
                start = prev_end + 1
            prev_end = end
        return res

    def make_splits(records, splits):
        split_current_ind = 0
        session_ind_current = 0
        session_seq_current = 0
        for ind, record in enumerate(records):
            if split_current_ind < len(splits) and ind > splits[split_current_ind]:
                split_current_ind += 1
                session_ind_current += 1
                session_seq_current = 0
            record['session_id'] = record['session_id'] + '_' + str(session_ind_current)
            record['session_sequence'] = session_seq_current
            session_seq_current += 1
        return records

    for key, records in groups:
        records = [r.to_dict() for r in records]
        # ignore too short sessions
        if len(records) < min_len:
            continue
        # not change sessions small enough
        if len(records) <= max_len:
            for r in records:
                yield Record(**r)
            continue
        records = sorted(records, key=lambda i: i['server_time_ms'])

        time_current = [record['server_time_ms'] for record in records[1:]]
        time_prev = [record['server_time_ms'] for record in records[:-1]]
        time_deltas = [time_current[i] - time_prev[i] for i in range(len(records) - 1)]
        # ignore tech events for splitting
        time_deltas = [(i, d) for i, d in enumerate(time_deltas)
                       if records[i + 1].get("input_type") != "tech"]
        splits = sorted(split_by_big_delta(time_deltas, min_len, max_len))
        splits = skip_unnecessary_splits(splits, len(time_deltas), max_len)
        splits = [time_deltas[s][0] for s in splits]

        records = make_splits(records, splits)
        for r in records:
            yield Record(**r)


def main(input_path=None, pool=None, tmp_path=None, output_table=None, memory_limit=8192, min_len=2, max_len=15):

    templates = {"job_root": "//tmp/robot-voice-qa"}
    if tmp_path:
        templates['tmp_files'] = tmp_path

    cluster = hahn_with_deps(
                pool=pool,
                templates=templates,
                neighbours_for=__file__,
                include_utils=True
              )
    job = cluster.job().env(default_memory_limit=memory_limit)

    splitted_sessions = job.table(input_path) \
        .groupby('session_id') \
        .reduce(partial(split_sessions, min_len=min_len, max_len=max_len))

    splitted_sessions.put(output_table, schema=SCHEMA)

    job.run()

    return {"cluster": "hahn", "table": output_table}


if __name__ == '__main__':
    call_as_operation(main)
