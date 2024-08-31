#!/usr/bin/env python
# -*-coding: utf8 -*-
from os import path
from collections import OrderedDict
from nile.api.v1 import (
    extractors as ne,
    aggregators as na,
    clusters,
    Record,
    with_hints,
)
from utils.nirvana.op_caller import call_as_operation
from utils.yt.dep_manager import hahn_with_deps
from nile.api.v1.files import LocalFile, DevelopPackage
from qb2.api.v1 import typing
from nile.api.v1.datetime import date_range

SCHEMA = OrderedDict([
    ("intent", typing.Optional[typing.String]),
])

@with_hints(output_schema=SCHEMA)
def map_events(records):
    from usage_fields import extract_intent, get_slot_name
    for rec in records:
        new_record = {}
        slot_name = get_slot_name(rec.form_name or '')
        new_record['intent'] = extract_intent(rec.form_name or '', rec.form or {}, rec.response, rec.callback_args, rec.response.get('cards'), rec.analytics_info, None, slot_name, {})
        yield Record(**new_record)

def prepare_table(date, pool, sessions_root, output_root):

#    cluster = hahn_with_deps(
#        pool=pool,
#        neighbours_for=__file__,
#        include_utils=True
#    )
    cluster = clusters.Hahn(pool=pool).env(files=[LocalFile('operations/dialog/sessions/usage_fields.py')])
    job = cluster.job()

    session_events = (job.table(path.join(sessions_root, date))
                        .take(1000)
                        .map(map_events)
                        .put(path.join(output_root, date), schema=SCHEMA))
    job.run()


def main(date, pool = "voice",
         sessions_root='//home/voice/vins/logs/dialogs',
         output_root='//home/voice/nstbezz/test_deps'):

    if isinstance(date, (list, tuple)):
        start_date, end_date = date
        for date in date_range(start_date, end_date):
            prepare_table(date, pool, sessions_root, output_root)
        return {'out_first_path': path.join(output_root, start_date),
                'out_last_path': path.join(output_root, end_date)}

    prepare_table(date, pool, sessions_root, output_root)
    return {'out_path': path.join(output_root, date)}


if __name__ == '__main__':
    call_as_operation(main)
    # main('2020-01-01') # local run
