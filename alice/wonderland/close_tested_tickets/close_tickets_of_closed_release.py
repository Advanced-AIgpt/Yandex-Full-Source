#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import json
import requests

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


MASTER_QUERY = '?fields=key&expand=links&query=Queue:ALICERELEASE AND Status:Closed AND Resolution:Fixed AND Created:>today()-20w'
SLAVE_QUEUE_LIST = ['ALICE', 'DIALOG', 'ASSISTANT', 'MEGAMIND', 'IOT', 'QUASARUI', 'PASKILLS', 'CENTAUR']
CHECK_LINKS_QUERY = "?fields=key&query=Queue:ALICERELEASE AND Status:!Closed AND Relates:"

###########################

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

def get_closed_release_tickets():
    resp = requests.get(STARTREK_API_URL + MASTER_QUERY,
                        headers=STARTREK_ROBOT_HEADER)
    list_tickets = resp.json()
    return list_tickets

def find_bugs(list_prod_release_tickets):
    assert isinstance(list_prod_release_tickets, list)
    slave_url_list = []
    for ticket in list_prod_release_tickets:
        print('Closed and fixed release ' + ticket.get('key'))
        links = ticket.get('links')
        if not links:
            print('No linked tickets')
        else:
            for link in links:
                link_ticket_url = str(link['self'])
                slave_url_list.append(link_ticket_url)
    return slave_url_list

def decore(fn):
    def wrapper(ticket_list):
        list1 = []
        print('start decorator')
        for i in ticket_list:
            print(i)
            slave_ticket_key = fn(i)
            list1.append(slave_ticket_key)
            list1 = [x for x in list1 if x is not None]
            list1 = list(set(list1))
        return list1
    return wrapper

@decore
def convert_url_to_key(link_ticket_url):
    assert isinstance(link_ticket_url, str)
    print('Check ' + link_ticket_url)
    resp = requests.get(link_ticket_url, headers=STARTREK_ROBOT_HEADER)
    ticket_key = str(resp.json()['object']['key'])
    ticket_queue = ticket_key.split('-')[0]
    if ticket_queue in SLAVE_QUEUE_LIST:
        return ticket_key

@decore
def seek_link_to_not_closed_release(ticket_key):
    assert isinstance(ticket_key, str)
    print('Check ' + ticket_key)
    resp = requests.get(STARTREK_API_URL + CHECK_LINKS_QUERY + ticket_key,
                    headers=STARTREK_ROBOT_HEADER)
    if not resp.json():
        return ticket_key

def has_quasar_components(components):
    quasar = ["Quasar Linux Team", "Quasar Android Team"]
    names = list(map(lambda x: x['display'], components))
    names = list(filter(lambda x: x in quasar, names))
    if names:
        return True

@decore
def check_status_and_queue(ticket_key):
    assert isinstance(ticket_key, str)
    resp = requests.get(STARTREK_API_URL + '/' + ticket_key + '?fields=key,components,status', headers=STARTREK_ROBOT_HEADER)
    print('Check ' + ticket_key)
    ticket_queue = ticket_key.split('-')[0]
    ticket_status = resp.json()['status']['key']
    ticket_components = resp.json()['components']
    print(ticket_components)
    if not has_quasar_components(ticket_components):
        if ticket_status == 'tested' and ticket_queue == 'DIALOG':
            transition_step(ticket_key, 'rc')
        if ticket_status in ['rc', 'tested'] and ticket_queue in SLAVE_QUEUE_LIST:
            return(ticket_key)

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
    global tested_tickets_count
    list_closed_release_tickets = get_closed_release_tickets()
    if len(list_closed_release_tickets) == 0:
        write_logs(str('А, сегодня нечего закрывать.'))
    else:
        slave_url_list = find_bugs(list_closed_release_tickets)
        slave_tickets_list = convert_url_to_key(slave_url_list)
        slave_tickets_list = seek_link_to_not_closed_release(slave_tickets_list)
        slave_tickets_list = check_status_and_queue(slave_tickets_list)
        for ticket_key in slave_tickets_list:
            ticket_queue = ticket_key.split('-')[0]
            if ticket_queue in ['QUASARUI', 'PASKILLS']:
                transition_step(ticket_key, 'close')
            else:
                transition_step(ticket_key, 'closed')
            tested_tickets_count += 1
        if tested_tickets_count == 0:
            write_logs(str('А, сегодня нечего закрывать.'))
        else:
            write_logs('Нашёл и закрыл ' + str(tested_tickets_count))

tested_tickets_count = 0
log_comment = str('Ищу протестированные тикеты в закрытых релизах...\n')
work()
post_yamb_message(log_comment)
