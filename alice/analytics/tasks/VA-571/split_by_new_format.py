from basket_configs import get_basket_param
from collections import OrderedDict
from nile.api.v1 import Record, filters as nf, extractors as ne
from qb2.api.v1 import typing
from utils.nirvana.op_caller import call_as_operation
from utils.yt.dep_manager import hahn_with_deps
import utils.yt.basket_common as common
from functools import partial

SCHEMA = OrderedDict([
    ("activation_type", str),
    ("additional_options", typing.Json),
    ("app_preset", str),
    ("asr_options", typing.Json),
    ("asr_text", str),
    ("basket", str),
    ("client_time", str),
    ("context_len", int),
    ("device_state", typing.Json),
    ("exact_location", str),
    ("experiments", typing.Json),
    ("fetcher_mode", str),
    ("full_text", str),
    ("is_empty_asr", bool),
    ("is_good_session", bool),
    ("is_new", str),
    ("is_positive", str),
    ("is_positive_prob", float),
    ("location", typing.Json),
    ("mds_key", str),
    ("megamind_request_text", str),
    ("new_format_general_current", bool),
    ("oauth", str),
    ("real_generic_scenario", str),
    ("real_reqid", str),
    ("real_session_id", str),
    ("real_session_sequence", int),
    ("real_uuid", str),
    ("request_id", str),
    ("request_source", str),
    ("reversed_session_sequence", int),
    ("sampling_intent", str),
    ("session_id", str),
    ("session_len", int),
    ("session_sequence", int),
    ("text", str),
    ("timezone", str),
    ("toloka_intent", str),
    ("ts", int),
    ("vins_intent", str),
    ("voice_binary", str),
    ("voice_url", str)
])


def get_format(records, override_basket_params=None, parse_iot=False, new_format_general=False):
    for record in records:
        rec = record.to_dict()
        if 'basket' not in rec or rec['basket'] == 'input_basket':
            rec['new_format_general_current'] = new_format_general
        else:
            rec['new_format_general_current'] = get_basket_param('new_format_general', basket_alias=rec['basket'],
                                                       override_basket_params=override_basket_params)
        if rec['new_format_general_current'] is None:
            rec['new_format_general_current'] = False
        yield Record(**rec)


def main(input_table=None, pool=None, tmp_path=None, old_format_table=None, new_format_table=None,
         override_basket_params=None, parse_iot=False, new_format=True, new_format_general=False):
    templates = {"job_root": "//tmp/robot-voice-qa"}
    if tmp_path:
        templates['tmp_files'] = tmp_path
    cluster = hahn_with_deps(
                pool=pool,
                templates=templates,
                neighbours_for=__file__,
                neighbour_names=['basket_configs.py'],
                include_utils=True
              )

    job = cluster.job()

    table_with_new_format = job.table(input_table) \
        .map(partial(
        get_format,
        override_basket_params=override_basket_params,
        parse_iot=parse_iot,
        new_format_general=new_format_general
    ))

    table_with_new_format_old = table_with_new_format.filter(nf.equals('new_format_general_current', False))
    table_with_new_format_new = table_with_new_format.filter(nf.equals('new_format_general_current', True))

    table_with_new_format_old = table_with_new_format_old.project(ne.all(exclude=('_other')))
    table_with_new_format_new = table_with_new_format_new.project(ne.all(exclude=('_other')))

    old_format_table = old_format_table or "//tmp/robot-voice-qa/fix_basket_quasar_task_" + common.random_part(16)
    table_with_new_format_old.put(old_format_table, schema=SCHEMA)
    new_format_table = new_format_table or "//tmp/robot-voice-qa/fix_basket_quasar_task_" + common.random_part(16)
    table_with_new_format_new.put(new_format_table, schema=SCHEMA)

    job.run()

    return [{"cluster": "hahn", "table": old_format_table}, {"cluster": "hahn", "table": new_format_table}]


if __name__ == '__main__':
    call_as_operation(main)
