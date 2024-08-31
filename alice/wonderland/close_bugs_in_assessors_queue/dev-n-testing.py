#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import json
import requests
import random

import sys
reload(sys)
sys.setdefaultencoding('utf-8')

###########################
# блок констант и токенов #
###########################

ST_API_URL = 'https://st-api.yandex-team.ru/v2/issues'
ST_TOKEN = os.getenv("STARTREK_OAUTH_TOKEN")

YAMB_API_URL = 'https://bp.mssngr.yandex.net/bot'
YAMB_TOKEN = os.getenv("YAMB_TOKEN")
YAMB_CHAT_ID = os.getenv("YAMB_CHAT_ID")

MASTER_QUEUE = 'ALICE'
SLAVE_QUEUE = 'ALICEASSESSORS'

QUERY = 'Queue: ALICE AND "Current Stage": Development, Testing AND Status: ! Closed AND Updated: week()'

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
                        headers={'Authorization': 'OAuthTeam %s' % (YAMB_TOKEN)})
    try:
        return resp.json()['message']
    except KeyError:
        print('Не удалось отправить комментарий\n' + str(resp.json()))
    print(resp.json())

def get_devtesting_tickets_from_master():
    page = 1
    page_list_len = -1
    list_closed_tickets = []
    while page_list_len != 0:
        resp = requests.get('%s?query=%s&expand=links&page=%d' % (ST_API_URL, QUERY, page),
                            headers={'Authorization': 'OAuth %s' % (ST_TOKEN)})
        page_list_len = len(resp.json())
        if resp.json():
            list_closed_tickets.extend(resp.json())
        print(page, page_list_len)
        page += 1
    return list_closed_tickets


def get_ticket_body(ticket_id):
    assert isinstance(ticket_id, str)
    resp = requests.get(ST_API_URL + '/' + ticket_id + '?fields=resolution&expand=links',
                        headers={'Authorization': 'OAuth %s' % (ST_TOKEN)})
    ticket_body = resp.json()
    return ticket_body


def check_status_and_queue(link_ticket_url,queue):
    assert isinstance(link_ticket_url, str)
    resp = requests.get(link_ticket_url, headers={'Authorization': 'OAuth %s' % (ST_TOKEN)})

    ticket_key = str(resp.json()['object']['key'])
    ticket_queue = ticket_key.split('-')[0]
    ticket_status = resp.json()['status']['key']

    if ticket_status != 'closed' and ticket_queue == queue:
        return(ticket_key)


def post_comment(ticket_key):
    assert isinstance(ticket_key, str)
    text = 'Закрываю, поскольку баг найден в окружении testing/development. Если воспроизведётся, обязательно переоткрыть и поставить кейсу FAILED. \
            \n Это нужно, чтобы тестировщики обращали внимание и не пропускали в прод как known-баги.'
    print(ticket_key, text)
    comment = json.dumps({"text": text}, default=str)
    resp = requests.post('%s/%s/comments' % (ST_API_URL, ticket_key),
                         data=comment,
                         headers={'Authorization': 'OAuth %s' % (ST_TOKEN)})
    try:
        return resp.json()['self']
    except KeyError:
        write_logs('Не удалось отправить комментарий\n' + str(resp.json()))


def close_slave_ticket(slave_ticket_key):
    assert isinstance(slave_ticket_key, str)
    new_resolution = json.dumps({"resolution": "fixed"}, ensure_ascii=False, default=str)
    resp = requests.post('%s/%s/transitions/close/_execute' % (ST_API_URL, slave_ticket_key),
                         data=new_resolution,
                         headers={'Authorization': 'OAuth %s' % (ST_TOKEN)})
    write_logs('https://st.yandex-team.ru/' + slave_ticket_key + ' закрыт')
    try:
        return resp.json()[0]['self']
    except KeyError:
        write_logs('Не удалось закрыть тикет\n' + str(resp.json()))


def find_slave_tickets(list_closed_tickets):
    assert isinstance(list_closed_tickets, list)
    for ticket in list_closed_tickets:
        master_links = ticket.get('links')

        if master_links:
            for link in master_links:
                link_ticket_url = str(link['self'])
                slave_ticket_key = check_status_and_queue(link_ticket_url,SLAVE_QUEUE)

                if slave_ticket_key:
                    global devtesting_count
                    devtesting_count += 1
                    close_slave_ticket(slave_ticket_key)
                    post_comment(slave_ticket_key)

def work():
    list_devtesting_tickets = get_devtesting_tickets_from_master()
    if list_devtesting_tickets:
        find_slave_tickets(list_devtesting_tickets)
    if devtesting_count:
        write_logs('Нашёл и закрыл ' + str(devtesting_count))
    else:
        write_logs('Новых не завезли.')


devtesting_count = 0
log_comment = str('Ищу testing/development баги в ALICEASSESSORS...\n')
work()
post_yamb_message(log_comment)
