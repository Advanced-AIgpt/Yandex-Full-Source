import json
import logging
import os
import re
import requests
import time
from datetime import datetime

import reactor_client.reactor_objects as r_objects
from reactor_client.reactor_api import ReactorAPIClientV1


REACTOR_URL = 'https://reactor.yandex-team.ru'
STARTREK_URL = 'https://st-api.yandex-team.ru/v2'
INFRA_URL = 'https://infra-api.yandex-team.ru/v1'

INFRA_SERVICE_ID = 1894
INFRA_ENV_ID = 3011

TIMESTAMP_WHOLE_DAY = 86400


def timestamp_to_date(ts):
    return datetime.fromtimestamp(ts).strftime('%Y-%m-%d')


def date_to_timestamp(date):
    res = time.mktime(datetime.strptime(date, '%Y-%m-%d').timetuple())
    return int(res)


def add_day(date):
    ts = date_to_timestamp(date) + TIMESTAMP_WHOLE_DAY
    return timestamp_to_date(ts)


def get_startrek_tickets_ids(reactor_path):
    # Reactor token is same as Nirvana token
    client = ReactorAPIClientV1(base_url=REACTOR_URL, token=os.environ['REACTOR_TOKEN'])

    artifact_id = r_objects.ArtifactIdentifier(
        namespace_identifier=r_objects.NamespaceIdentifier(namespace_path=reactor_path),
    )

    flt = r_objects.ArtifactInstanceFilterDescriptor(
        artifact_id,
    )

    ids = []
    for instance in client.artifact_instance.range(flt):
        ids.append(instance.metadata.dict_obj['value'])
    return ids


def get_startrek_ticket(ticket_id):
    url = STARTREK_URL + '/issues/' + ticket_id
    response = requests.get(url=url, headers={'Authorization': 'OAuth ' + os.environ['STARTREK_TOKEN']})
    return json.loads(response.text)


def parse_startrek_ticket_full_date(ticket):
    desc = ticket['description']
    m = re.search(r'\*\*Full dates\*\*.*?(?P<start>\d{4}-\d{2}-\d{2}).*?(?P<finish>\d{4}-\d{2}-\d{2})', desc)
    if not m:
        return None
    if 'start' not in m.groupdict():
        logging.warn('Can not find start date!')
        return None
    elif 'finish' not in m.groupdict():
        logging.warn('Can not find finish date!')
        return None
    else:
        return (m['start'], m['finish'])


def get_startrek_tickets_full_dates(tickets_ids):
    res = dict()
    for ticket_id in tickets_ids:
        ticket = get_startrek_ticket(ticket_id)
        full_date = parse_startrek_ticket_full_date(ticket)
        if full_date:
            res[ticket_id] = full_date
    return res


def get_infra_events():
    url = INFRA_URL + '/events?serviceId=' + str(INFRA_SERVICE_ID)
    resp = requests.get(url=url, headers={'Authorization': 'OAuth ' + os.environ['INFRA_TOKEN']})
    return json.loads(resp.text)


def is_dates_correct(event, date):
    event_start_time = timestamp_to_date(event['start_time'])
    event_finish_time = timestamp_to_date(event['finish_time'])

    real_start_time, real_finish_time = date

    if event_start_time != real_start_time:
        logging.warn('Wrong start time, recreating!')
        return False
    if event_finish_time != real_finish_time:
        logging.warn('Wrong finish time, recreating!')
        return False
    return True


def create_event(title, dates):
    logging.info('creating event %s' % title)

    start_date, finish_date = dates

    url = INFRA_URL + '/events'
    data = {
        'title': title,
        'description': 'Experiment from ticket {}'.format(title),
        'serviceId': INFRA_SERVICE_ID,
        'environmentId': INFRA_ENV_ID,
        'startTime': str(date_to_timestamp(start_date)),
        'finishTime': str(date_to_timestamp(add_day(finish_date)) - 1),
        'type': 'maintenance',
        'severity': 'minor',
        'sendEmailNotifications': False,
        'setAllAvailableDc': True,
        'tickets': title,
    }
    resp = requests.post(url=url, data=json.dumps(data), headers={'Authorization': 'OAuth ' + os.environ['INFRA_TOKEN'], 'Content-Type': 'application/json'})
    logging.info(resp.text)


def delete_event(event):
    logging.info('deleting event %s' % event['title'])
    url = INFRA_URL + '/events/' + str(event['id'])
    resp = requests.delete(url=url, headers={'Authorization': 'OAuth ' + os.environ['INFRA_TOKEN']})
    logging.info(resp.text)


def publish_dates(dates):
    events = get_infra_events()
    event_titles = {event['title'] for event in events}

    for event in events:
        title = event['title']
        if title in dates:
            if not is_dates_correct(event, dates[title]):
                delete_event(event)
                create_event(title, dates[title])

    for key in dates:
        if key not in event_titles:
            create_event(key, dates[key])


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')

    tickets_ids = get_startrek_tickets_ids(reactor_path='/home/robot-eksperimentus/AdmTaskForVoice')
    full_dates = get_startrek_tickets_full_dates(tickets_ids)
    publish_dates(full_dates)
