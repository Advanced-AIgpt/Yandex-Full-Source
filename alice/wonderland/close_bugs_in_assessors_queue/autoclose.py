#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import json
import requests
import random

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

MASTER_QUEUE = 'ALICE'
SLAVE_QUEUE = 'ALICEASSESSORS'

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

def get_count_closed_tickets_from_master():
    resp = requests.get(STARTREK_API_URL + '/_count?filter=queue:' + MASTER_QUEUE + '&filter=status:closed&filter=resolved:today()',
                        headers=STARTREK_ROBOT_HEADER)
    count_closed_tickets = resp.json()
    return count_closed_tickets

def get_list_closed_tickets_from_master():
    page = 1
    page_list_len = -1
    list_closed_tickets = []
    while page_list_len != 0:
        resp = requests.get(STARTREK_API_URL + '?filter=queue:' + MASTER_QUEUE + '&filter=status:closed&filter=resolved:today()&expand=links&fields=resolution&page=' + str(page),
                            headers=STARTREK_ROBOT_HEADER)
        page_list_len = len(resp.json())
        if resp.json():
            list_closed_tickets.extend(resp.json())
        print(page, page_list_len)
        page += 1
    return list_closed_tickets

def get_ticket_body(ticket_id):
    assert isinstance(ticket_id, str)
    resp = requests.get(STARTREK_API_URL + '/' + ticket_id + '?fields=resolution&expand=links',
                        headers=STARTREK_ROBOT_HEADER)
    ticket_body = resp.json()
    return ticket_body

def check_slave_ticket(slave_ticket_key):
    assert isinstance(slave_ticket_key, str)
    ticket_body = get_ticket_body(slave_ticket_key)
    links = ticket_body.get('links')
    # print(links)
    if len(links) > 1:
        for link in links:
            link_ticket_url = str(link['self'])
            open_master_ticket = check_status_and_queue(link_ticket_url,MASTER_QUEUE)
            if open_master_ticket is not None:
                write_logs('Призвал тестеров в тикет https://st.yandex-team.ru/' + slave_ticket_key + '. Там привязан незакрытый баг ' + open_master_ticket)
                post_comment(slave_ticket_key)
                slave_ticket_key = ''
                break
    return slave_ticket_key

def check_status_and_queue(link_ticket_url,queue):
    assert isinstance(link_ticket_url, str)
    resp = requests.get(link_ticket_url, headers=STARTREK_ROBOT_HEADER)
    ticket_key = str(resp.json()['object']['key'])
    ticket_queue = ticket_key.split('-')[0]
    ticket_status = resp.json()['status']['key']
    if ticket_status != 'closed' and ticket_queue == queue:
        return(ticket_key)

def post_comment(ticket_key):
    assert isinstance(ticket_key, str)
    testers = ['osennikovak', 'kr1kaqt', 'bashev-da', 'kapitonovaj']
    randnum = random.randrange(0, 3, 1)
    text = 'Здесь привязан и открытый, и закрытый тикеты очереди ' + MASTER_QUEUE + '. Посмотрите, нужно ли закрыть этот.'
    comment = json.dumps({"text": text, 'summonees': testers[randnum]}, default=str)
    resp = requests.post('%s/%s/comments' % (STARTREK_API_URL, ticket_key),
                         data=comment,
                         headers=STARTREK_ROBOT_HEADER)
    try:
        return resp.json()['self']
    except KeyError:
        write_logs('Не удалось отправить комментарий\n' + str(resp.json()))

def close_slave_ticket(slave_ticket_key,master_resolution):
    assert isinstance(slave_ticket_key, str)
    assert isinstance(master_resolution, str)
    new_resolution = json.dumps({"resolution": master_resolution}, ensure_ascii=False, default=str)
    resp = requests.post('%s/%s/transitions/close/_execute' % (STARTREK_API_URL, slave_ticket_key),
                         data=new_resolution,
                         headers=STARTREK_ROBOT_HEADER)
    write_logs('https://st.yandex-team.ru/' + slave_ticket_key + ' закрыт с резолюцией ' + master_resolution)
    try:
        return resp.json()[0]['self']
    except KeyError:
        write_logs('Не удалось закрыть тикет\n' + str(resp.json()))


def find_slave_tickets(list_closed_tickets):
    assert isinstance(list_closed_tickets, list)
    for ticket in list_closed_tickets:
        if ticket['resolution'] is None:
            write_logs('У этого тикета нет резолюции ' + str(ticket['key']))
            master_resolution = str('fixed')
        # есть прецеденты закрытых тикетов без резолюции, поэтому ставим дефолтную
        else:
            master_resolution = str(ticket['resolution']['key'])
        master_links = ticket.get('links')
        if master_links:
            for link in master_links:
                link_ticket_url = str(link['self'])
                slave_ticket_key = check_status_and_queue(link_ticket_url,SLAVE_QUEUE)
                if slave_ticket_key:
                    slave_ticket_key = check_slave_ticket(slave_ticket_key)
                    if slave_ticket_key:
                        global tested_tickets_count
                        tested_tickets_count += 1
                        close_slave_ticket(slave_ticket_key,master_resolution)

def work():
    count_closed_tickets = get_count_closed_tickets_from_master()
    if count_closed_tickets == 0:
        write_logs('А, сегодня нечего закрывать.')
    else:
        list_closed_tickets = get_list_closed_tickets_from_master()
        find_slave_tickets(list_closed_tickets)
        if tested_tickets_count == 0:
            write_logs('А, сегодня нечего закрывать.')
        else:
            write_logs('Нашёл и закрыл ' + str(tested_tickets_count))

tested_tickets_count = 0
log_comment = str('Начинаю закрывать тикеты ALICEASSESSORS...\n')
work()
post_yamb_message(log_comment)
