# -*-coding: utf8 -*-
from nile.api.v1 import (
    clusters,
    Record
)

from utils.nirvana.op_caller import call_as_operation


def flatten(records):
    for record in records:
        parsed_record = record.to_dict()
        parsed_record['id'] = record.get('id', '')
        parsed_record['skill_id'] = record.get('skill_id', '')
        parsed_record['app'] = record.get('app', '')
        for replica in record['skill_session']:
            if replica:
                parsed_record['replica'] = replica
                parsed_record['req_id'] = replica.get('req_id')
                parsed_record['replica']['session_part'] = 'skill'
                yield Record.from_dict(parsed_record)
        for replica in record['before_session']:
            if replica:
                parsed_record['replica'] = replica
                parsed_record['req_id'] = replica.get('req_id')
                parsed_record['replica']['session_part'] = 'before'
                yield Record.from_dict(parsed_record)
        for replica in record['after_session']:
            if replica:
                parsed_record['replica'] = replica
                parsed_record['req_id'] = replica.get('req_id')
                parsed_record['replica']['session_part'] = 'after'
                yield Record.from_dict(parsed_record)


def update(records):
    for record in records:
        parsed_record = record.to_dict()
        if record.get('query'):
            parsed_record['replica']['_query'] = record['query']
        if record.get('reply'):
            parsed_record['replica']['_reply'] = record['reply']
        yield Record.from_dict(parsed_record)


def main(table_with_filtered_text='//tmp/alexslinka/clear_personal_info/sessions_filtered',
         output_path='//home/voice/alexslinka/clear_personal_info/result', date=None,
         sessions_to_join='//tmp/alexslinka/clear_personal_info/sessions_for_sandbox_to_join', pool=None):
    if pool is None:
        cluster = clusters.Hahn()
    else:
        cluster = clusters.Hahn(pool=pool)

    job = cluster.job('restore sessions')

    filtered = job.table(table_with_filtered_text + '/' + date)

    job.table(sessions_to_join + '/' + date) \
        .map(flatten)\
        .join(filtered, by='req_id', assume_unique_right=True, type='left') \
        .map(update) \
        .put(output_path + '/' + date)
    job.run()
    return {"cluster": "hahn", "table": output_path}


if __name__ == '__main__':
    call_as_operation(main)
