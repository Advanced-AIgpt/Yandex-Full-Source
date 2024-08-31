#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import json
import requests
import random
from datetime import datetime, timedelta
import sys
reload(sys)
sys.setdefaultencoding('utf-8')

###########################
# блок констант и токенов #
###########################

STARTREK_API_URL = 'https://st-api.yandex-team.ru/v2/issues'
STARTREK_ROBOT_TOKEN = os.getenv("STARTREK_OAUTH_TOKEN")
STARTREK_ROBOT_HEADER = {'Authorization': 'OAuth %s' % (STARTREK_ROBOT_TOKEN)}

YAMB_API_URL = 'https://bp.mssngr.yandex.net/bot'
YAMB_TOKEN = os.getenv("YAMB_TOKEN")
YAMB_CHAT_ID = os.getenv("YAMB_CHAT_ID")
YAMB_HEADER = {'Authorization': 'OAuthTeam %s' % (YAMB_TOKEN)}

MASTER_QUERY = '?expand=links&query=Queue: ALICERELEASE AND Status: Production AND Components: ! "Релиз флагов"'
SLAVE_QUEUE_LIST = ['ALICE', 'DIALOG', 'ASSISTANT', 'MEGAMIND', 'IOT', 'QUASARUI', 'PASKILLS', 'CENTAUR'] # вынесла 'QUASAR' пока не определимся с версиями

def write_logs(string):
    global log_comment
    print(string)
    log_comment += string + '\n'
    return log_comment

def post_yamb_message(log_comment):
    assert isinstance(log_comment, str)
    message = {"chat_id": YAMB_CHAT_ID, "text": log_comment}
    print(message)
    resp = requests.post( YAMB_API_URL + '/sendMessage/',
                        data=message,
                        headers=YAMB_HEADER)
    try:
        return resp.json()['message']
    except KeyError:
        print('Не удалось отправить комментарий\n' + str(resp.json()))
    print(resp.json())

def get_prod_release_tickets():
    list_tickets = []
    resp = requests.get(STARTREK_API_URL + MASTER_QUERY,
                        headers=STARTREK_ROBOT_HEADER)
    if resp.json():
        list_tickets.extend(resp.json())
    return list_tickets

def find_bugs(list_prod_release_tickets):
    assert isinstance(list_prod_release_tickets, list)

    link_list = []
    for ticket in list_prod_release_tickets:
        write_logs('Production release ' + str(ticket.get('key')))
        links = ticket.get('links', [])
        if not links: write_logs('No linked tickets')
        link_list.extend(links)

    link_list   = list(map(lambda link: str(link['self']), link_list))
    ticket_list = list(map(check_status_and_queue, link_list))
    ticket_list = list(filter(lambda x: x is not None, ticket_list))
    ticket_list = list(map(check_components, ticket_list))
    ticket_list = list(filter(lambda x: x is not None, ticket_list))

    global tested_tickets_count
    for ticket in ticket_list:
        ticket_queue = ticket.split('-')[0]
        if ticket_queue in ['QUASARUI', 'PASKILLS', 'CENTAUR']:
            transition_step(ticket, 'close')
        else:
            transition_step(ticket, 'closed')
        tested_tickets_count += 1

def check_components(ticket_key):
    print(ticket_key)
    resp = requests.get(STARTREK_API_URL + '/' + ticket_key + '?fields=key,components', headers=STARTREK_ROBOT_HEADER)
    print('Check components', ticket_key)
    components = resp.json().get('components', [])

    quasar = ['SK Team', 'Quasar Android Team']
    names = list(map(lambda x: str(x['display']), components))
    names = list(filter(lambda x: x in quasar, names))

    if not names:
        return ticket_key

def check_status_and_queue(link_ticket_url):
    assert isinstance(link_ticket_url, str)
    resp = requests.get(link_ticket_url, headers=STARTREK_ROBOT_HEADER)
    ticket_key = str(resp.json()['object']['key'])
    print('Check ' + ticket_key)

    ticket_queue = ticket_key.split('-')[0]
    ticket_status = resp.json()['status']['key']

    if ticket_status == 'tested' and ticket_queue == 'DIALOG':
        transition_step(ticket_key, 'rc')
    if ticket_status in ['tested', 'rc'] and ticket_queue in SLAVE_QUEUE_LIST:
        return ticket_key
    if ticket_queue == 'ALICE' and ticket_status != 'closed':
        check_not_showstopper(ticket_key)


def is_older_than_week(created):
    today_gmt = datetime.now() - timedelta(hours=3)
    created_date = datetime.strptime(created[:19], "%Y-%m-%dT%H:%M:%S")
    return (today_gmt - created_date).days >= 14

def check_not_showstopper(ticket_key):
    assert isinstance(ticket_key, str)
    resp = requests.get(STARTREK_API_URL + '/' + ticket_key + '?fields=created,detectionStage,currentStage',
                        headers=STARTREK_ROBOT_HEADER)
    detectionStage = resp.json().get('detectionStage')
    currentStage = resp.json().get('currentStage')
    created = resp.json().get('createdAt')
    if detectionStage == currentStage == 'Release' and is_older_than_week(created):
        write_logs('Release and not stopper https://st.yandex-team.ru/' + ticket_key)
        change_bug_stage(ticket_key)

def change_bug_stage(ticket_key):
    assert isinstance(ticket_key, str)
    new_currentStage = json.dumps({"currentStage": "Production"}, ensure_ascii=False)
    resp = requests.patch('%s/%s' % (STARTREK_API_URL, ticket_key),
                         data=new_currentStage,
                         headers=STARTREK_ROBOT_HEADER)
    try:
        return resp.json()['key']
    except KeyError:
        write_logs('Не удалось проставить текущую стадию https://st.yandex-team.ru/' + ticket_key + '\n' + str(resp.json()))

def transition_step(slave_ticket_key, action):
    assert isinstance(slave_ticket_key, str)
    assert isinstance(action, str)
    new_resolution = json.dumps({"resolution": 'fixed'}, ensure_ascii=False, default=str)
    resp = requests.post('%s/%s/transitions/%s/_execute' % (STARTREK_API_URL, slave_ticket_key, action),
                         data=new_resolution,
                         headers=STARTREK_ROBOT_HEADER)
    write_logs('https://st.yandex-team.ru/' + slave_ticket_key + ' ' + action)
    try:
        return resp.json()[0]['self']
    except KeyError:
        write_logs('Не удалось поменять тикет\n' + str(resp.json()))

def work():
    list_prod_release_tickets = get_prod_release_tickets()
    if len(list_prod_release_tickets) == 0:
        write_logs(str('А, сегодня нечего закрывать.'))
    else:
        find_bugs(list_prod_release_tickets)
        if tested_tickets_count == 0:
            write_logs('А, сегодня нечего закрывать.')
        else:
            write_logs('Нашёл и закрыл ' + str(tested_tickets_count))
            # здесь можно добавить строку про not_showstopper

tested_tickets_count = 0
log_comment = str('Ищу протестированные тикеты в production-релизах...\n')
work()
post_yamb_message(log_comment)
