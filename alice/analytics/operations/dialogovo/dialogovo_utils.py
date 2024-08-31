from nile.api.v1 import (
    clusters,
    Record,
    aggregators as na,
    statface as ns,
    filters as nf,
    extractors as ne
)
from qb2.api.v1 import filters as qf
from datetime import datetime


def get_skill_session_id(analytics_info):
    try:
        scenario_analytics_info = analytics_info['analytics_info'].get('Dialogovo', {}).get('scenario_analytics_info')
        for o in scenario_analytics_info['objects']:
            if 'skill_session' in o:
                return o['skill_session']['id']
    except:
        return None


def get_fielddate(ts):
    return datetime.fromtimestamp(ts).strftime("%Y-%m-%d")


def clone_with(field):
    kwargs = {
        field: "_total_"
    }
    def wrapper(records):
        for record in records:
            yield record
            yield Record(record, **kwargs)
    return wrapper


def find_object(objects, payload_key):
    for obj in objects:
        if payload_key in obj:
            return obj[payload_key]
    return None


def define_dev_type(dev_type):
    if dev_type in (1, 'External'):
        return 'external'
    elif dev_type in (2, 'Yandex'):
        return 'yandex'
    else:
        raise ValueError('Not a valid developer_type')


def get_skill_props(records):
    for record in records:
        skill_props = {}
        for o in record['analytics_info'].get('analytics_info', {}) \
                                         .get('Dialogovo', {}) \
                                         .get('scenario_analytics_info', {}) \
                                         .get('objects', []):
            if 'skill' in o:
                skill_props['category'] = o['skill']['category']
                skill_props['developer_type'] = define_dev_type(o['skill']['developer_type'])
                skill_props['developer_name'] = o['skill']['developer_name']
                skill_props['name'] = o['skill']['name']
                skill_props['channel'] = 'aliceSkill'
            if 'skill_session' in o and 'id' in o['skill_session']:
                skill_props['skill_session_id'] = o['skill_session']['id']
        yield Record(record, **skill_props)

