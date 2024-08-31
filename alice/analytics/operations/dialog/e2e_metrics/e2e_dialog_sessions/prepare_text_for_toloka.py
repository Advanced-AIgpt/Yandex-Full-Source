# -*-coding: utf8 -*-
from nile.api.v1 import (
    clusters,
    Record,
    with_hints,
    get_records_from_file,
    files as nfi,
)
from utils.nirvana.op_caller import call_as_operation
from operator import itemgetter


def parse_query(replica):
    if replica.get('callback') and replica['callback'].get('btn_data'):
        query = str(replica['callback']['btn_data']).replace('\"', '').decode('utf-8').strip()
        if ":" in query:
            query = query.split(':')[1]
        if query.endswith('}'):
            query = query[:-1]
        return query
    else:
        return replica.get('_query', '')


@with_hints(files=[nfi.TableFile('//home/paskills/skills/stable', filename='id_mapping')])
def prepare_text(records):
    map_ids = get_records_from_file('id_mapping')
    skill_id_mapping = dict()
    for record in map_ids:
        if str(record['onAir']) != '%false' and record.get('onAir') and record.get('name'):
            skill_id_mapping[record['id']] = record['name']
    for record in records:
        session = list()
        for replica in record['session']:
            replica['_query'] = parse_query(replica)
            reply = str(replica['_reply']).decode('utf-8').strip()
            reply = reply.replace('\n', ' ')
            if replica.get('cards'):
                for block in replica['cards']:
                    if block.get('type') == 'div_card' and block.get('actions'):
                        for skill in block['actions']:
                            if skill:
                                skill_parsed = skill.split('__')[-1]
                                if reply is None:
                                    reply = ''
                                if skill_parsed is not None and skill_parsed in skill_id_mapping:
                                    action = skill_id_mapping[skill_parsed].decode('utf-8')
                                    reply += '\n' + action
                                elif skill_parsed is not None:
                                    if skill_parsed == 'all_skills':
                                        reply += '\n' + 'Все навыки'.decode('utf-8')
                                    elif skill_parsed == 'serp':
                                        reply += '\n' + 'Поиск'.decode('utf-8')
                                    else:
                                        reply += '\n' + skill_parsed
            reply.replace('None', '').replace('\"', '"')
            replica['_reply'] = reply
            session.append(replica)
        yield Record(session=session, app=record['app'])


def make_parts_before_and_after_session(records):
    for record in records:
        session = {'before_session': list(), 'after_session': list(), 'skill_session': list()}
        for replica in record['session']:
            session[replica['session_part'] + '_session'].append(replica)
        yield Record(session=session, app=record['app'])


def main(input_path='//home/voice/alexslinka/e2e_dialogs/toloka_result',
         output_path='//tmp/robot-voice-qa/e2e_dialogs/toloka_result', date='2019-09-05', pool=None):
    if pool is None:
        cluster = clusters.Hahn()
    else:
        cluster = clusters.Hahn(pool=pool)

    job = cluster.job('prepare text')
    job.table(input_path + '/' + date)\
        .map(prepare_text) \
        .map(make_parts_before_and_after_session) \
        .put(output_path + '/' + date)
    job.run()
    return {"cluster": "hahn", "table": output_path}


if __name__ == '__main__':
    call_as_operation(main)
