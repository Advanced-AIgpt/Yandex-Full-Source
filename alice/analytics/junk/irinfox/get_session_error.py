# -*-coding: utf8 -*-
from os import path
from collections import OrderedDict
import time

from nile.api.v1 import (
    clusters,
    Record,
    grouping as ng,
    extractors as ne,
    datetime as nd,
    filters as nf,
)
import urlparse, json
from utils.nirvana.op_caller import call_as_operation


def list_cards(response):
    if not response or not response.get('cards'):
        return []
    return list(filter(lambda x: x is not None, map(_parse_card, response['cards'])))


def _parse_card(card):
    if 'type' not in card or 'text' not in card:
        return None
    props = {'text': card['text'],
             'type': card['type']}
    if card['type'] == 'div_card':
        props['actions'] = actions = []  # Имена действий
        props['card_id'] = None
        props['intent_name'] = None

        for card_id, intent_name, action_name in _parse_el(card['body']):
            if card_id is not None:
                props['card_id'] = card_id
            if intent_name is not None:
                props['intent_name'] = intent_name
            if action_name:
                action_name.append(action_name)

    return props


def _parse_el(element, only_log_id=False):
    if isinstance(element, dict):
        for key, val in element.iteritems():
            if key == "action":
                yield _parse_action(val, only_log_id)
            else:
                for act in _parse_el(val, only_log_id):
                    yield act
    elif isinstance(element, list):
        for sub_el in element:
            for act in _parse_el(sub_el, only_log_id):
                yield act
    else:
        pass


def _parse_action(action, only_log_id=False):
    parts = urlparse.urlsplit(action['url'])
    if parts.path.startswith('?'):
        # Некоторые версии urlparse не допускают пустого пути, а в экшенах он повсеместен
        query = urlparse.parse_qs(parts.path[1:])
    else:
        query = urlparse.parse_qs(parts.query)
    if only_log_id or 'directives' not in query:
        return None, None, None

    try:
        directives = json.loads(query['directives'][0])
    except ValueError:
        return 1, 1, 1
    for d in directives:
        if d['name'] == 'on_card_action':
            pl = d['payload']
            return (pl['card_id'],
                    pl['intent_name'],
                    pl.get('action_name') or pl.get('case_name'))
    return None, None, None  # Так и не встретился on_card_action


def requirements(record):
    urls = list()
    if record.get('response'):
        if record['response'].get('cards'):
            for card in record['response'].get('cards'):
                if 'body' in card:
                    if 'states' in card['body']:
                        for state in card['body']['states']:
                            if 'blocks' in state:
                                for block in state['blocks']:
                                    if 'action' in block:
                                        if 'url' in block['action']:
                                            urls.append(block['action']['url'])
    return urls


def custom_filter(records):
    for record in records:
        urls = requirements(record)
        for url in urls:
            yield Record(record, url=url)


def error(records):
    for record in records:
        url = record['url']
        parts = urlparse.urlsplit(url)
        if parts.path.startswith('?'):
            query = urlparse.parse_qs(parts.path[1:])
        else:
            query = urlparse.parse_qs(parts.query)
        try:
            try:
                json.loads(query['directives'][0])
            except ValueError:
                yield Record(record, error='ValueError')
        except KeyError or TypeError:
            yield Record(record, error='KeyOrTypeError')


def main(date, sessions_root,
         dialogs_root='//home/voice/vins/logs/dialogs', pool='voice'):

    sessions_path = path.join(sessions_root, date)
    dialogs_path = path.join(dialogs_root, date)
    cluster = clusters.YT(
        proxy="hahn.yt.yandex.net", pool=pool)
    job = cluster.job()

    job.table(dialogs_path)\
        .map(custom_filter) \
        .map(error) \
        .put(sessions_path)
    job.run()
    return {'sessions_path': sessions_path}


if __name__ == '__main__':
    call_as_operation(main)
