# !/usr/bin/env python
# encoding: utf-8

from utils.nirvana.op_caller import call_as_operation

from nile.api.v1 import (
    clusters,
    Record,
    with_hints
)
from qb2.api.v1 import extractors as ne

import re


@with_hints(output_schema={'sound': str, 'fielddate': str, 'skill_id': str})
def find_records_with_standart_sounds(records):
    for record in records:
        if ('response' in record and
                record['response'] is not None and
                'voice_text' in record['response'] and
                record['response']['voice_text'] is not None and
                "<speaker audio=\"alice-" in record['response']['voice_text']):

            skill_id = 'not_a_skill'

            if ('form' in record and
                    record['form'] is not None and
                    'slots' in record['form'] and
                    record['form']['slots'] is not None):
                for slot in record['form']['slots']:
                    if ('slot' in slot and
                            slot['slot'] == 'skill_id' and
                            'value' in slot and
                            slot['value'] is not None):
                        skill_id = slot['value']

            for category in re.findall('<speaker audio=\"alice-.{5,35}opus\">', record['response']['voice_text']):
                yield Record(fielddate=record['fielddate'],
                             skill_id=skill_id,
                             sound=category,
                             request_id=record['request'].get('request_id', ''))


def main(date):
    cluster = clusters.Hahn()
    job = cluster.job()

    job.table('//home/voice/vins/logs/dialogs/' + date) \
       .project(ne.all(), ne.const('fielddate', date)) \
       .map(find_records_with_standart_sounds) \
       .put('//home/voice/nikitachizhov/VA-447/' + date)

    job.run()


if __name__ == '__main__':
    call_as_operation(main)
