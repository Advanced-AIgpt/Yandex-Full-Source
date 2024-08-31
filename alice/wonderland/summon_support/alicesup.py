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

MASTER_QUEUE = 'ALICE'
SLAVE_QUEUE = 'ALICESUP'

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
    yamb_error = get_error_message(resp.json())
    try:
        return resp.json()['message']
    except KeyError:
        print('Не удалось отправить комментарий в Q. Причина: ' + yamb_error)

def get_error_message(resp_json):
    assert isinstance(resp_json, dict)
    errorMessage = resp_json.get('errorMessages',[u'Неизвестна'])
    errorMessage = errorMessage[0].encode('utf8')
    return errorMessage

def get_list_closed_tickets_from_master():
    page = 1
    page_list_len = -1
    list_closed_tickets = []
    while page_list_len != 0:
        resp = requests.get(STARTREK_API_URL + '?filter=queue:' + MASTER_QUEUE + \
                '&filter=status:closed&expand=links&filter=resolved:week()&page=' + str(page),
                headers=STARTREK_ROBOT_HEADER)
        page_list_len = len(resp.json())
        if resp.json():
            list_closed_tickets.extend(resp.json())
        print(page, page_list_len)
        page += 1
    return list_closed_tickets

def convert_url_to_key(link_ticket_url):
    assert isinstance(link_ticket_url, str)
    print('Check ' + link_ticket_url)
    resp = requests.get(link_ticket_url, headers=STARTREK_ROBOT_HEADER)
    ticket_key = str(resp.json()['object']['key'])
    print('Result ' + ticket_key)
    ticket_status = str(resp.json()['status']['key'])
    if ticket_status in ['inProgress', 'open'] :
        return ticket_key

def find_slave_tickets(list_closed_tickets):
    assert isinstance(list_closed_tickets, list)
    link_list = []
    for ticket in list_closed_tickets:
        master_links = ticket.get('links')
        if master_links: link_list.extend(master_links)

    ticket_list = []
    for link in link_list:
        link_ticket_url = str(link['self'])
        ticket_key = convert_url_to_key(link_ticket_url)
        if ticket_key: ticket_list.append(ticket_key)

    slave_ticket_list = []
    for ticket_key in ticket_list:
        ticket_queue = ticket_key.split('-')[0]
        if ticket_queue == SLAVE_QUEUE: slave_ticket_list.append(ticket_key)

    slave_ticket_list = list(set(slave_ticket_list))
    return slave_ticket_list

def transition_step(slave_ticket_key, action):
    assert isinstance(slave_ticket_key, str)
    assert isinstance(action, str)
    resp = requests.post('%s/%s/transitions/%s/_execute' % (STARTREK_API_URL, slave_ticket_key, action),
                         headers=STARTREK_ROBOT_HEADER)
    write_logs('https://st.yandex-team.ru/' + slave_ticket_key + ' ' + action)
    try:
        return resp.json()[0]['self']
    except KeyError:
        error = get_error_message(resp.json())
        write_logs('Не удалось поменять тикет\n' + error)

def work():
    list_closed_tickets = get_list_closed_tickets_from_master()
    ask_feedback_tickets = find_slave_tickets(list_closed_tickets)
    if len(ask_feedback_tickets) == 0:
        write_logs('А, ничего нового.')
    else:
        for slave_ticket_key in ask_feedback_tickets:
            transition_step(slave_ticket_key, 'waitingForFeedback')
        write_logs('Поменял ' + str(len(ask_feedback_tickets)) + ' тикетов')

log_comment = str('Начинаю обработку ALICESUP...\n')
work()
post_yamb_message(log_comment)
