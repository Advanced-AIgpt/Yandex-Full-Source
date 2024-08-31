#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import json
import requests

###########################
# блок констант и токенов #
###########################

STARTREK_API_URL = 'https://st-api.yandex-team.ru/v2/issues'
STARTREK_ROBOT_TOKEN = os.getenv("STARTREK_OAUTH_TOKEN") # личный токен или робота
STARTREK_ROBOT_HEADER = {'Authorization': 'OAuth %s' % (STARTREK_ROBOT_TOKEN)}

MASTER_QUEUE = 'ALICE'
SLAVE_QUEUE = 'TVANDROIDSUP'

SUMMONEES = ['natalyatru',
            'o-v-burygin',
            'av-alexeeva',
            'aogross',
            'maravilliossa']

###########################

def write_logs(string): # артефакт алертов в Ямб
    global log_comment
    print(string)
    log_comment += string + '\n'
    return log_comment

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
        resp = requests.get(STARTREK_API_URL + '?filter=queue:' + MASTER_QUEUE
                            + '&fields=resolution'
                            + '&filter=status:closed&filter=resolved:month()'
                            + '&expand=links&page='
                            + str(page),
                            headers=STARTREK_ROBOT_HEADER)
        page_list_len = len(resp.json())
        if resp.json():
            list_closed_tickets.extend(resp.json())
        print(page, page_list_len)
        page += 1
    return list_closed_tickets

def check_status_and_queue(link_ticket_url,queue):
    assert isinstance(link_ticket_url, str)
    resp = requests.get(link_ticket_url, headers=STARTREK_ROBOT_HEADER)
    ticket_key = str(resp.json()['object']['key'])
    ticket_queue = ticket_key.split('-')[0]
    ticket_status = resp.json()['status']['key']
    if ticket_status in ['open', 'inProgress'] and ticket_queue == queue:
        return(ticket_key)

def post_comment(ticket_key, comment, summonees=[]):
    comment = json.dumps({"text": comment, 'summonees': summonees}, default=str)
    resp = requests.post('%s/%s/comments' % (STARTREK_API_URL, ticket_key),
                         data=comment,
                         headers=STARTREK_ROBOT_HEADER)
    post_comment_error = get_error_message(resp.json())
    try:
        return resp.json()['self']
    except KeyError:
        write_logs('Не удалось отправить комментарий с призывом. Причина: ' + post_comment_error)

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

class Master(object):
    def __init__(self, key, resolution, master_links):
        self.key = key
        self.resolution = resolution
        self.master_links = master_links

    def get_resolution(self):
        a = self.resolution["key"] if self.resolution else "fixed" # у тикета может не быть резолюции
        self.resolution = a
        return self.resolution

    def get_slave_tickets(self):
        a = []
        for link in self.master_links:
            link_ticket_url = str(link['self'])
            slave_ticket_key = check_status_and_queue(link_ticket_url,SLAVE_QUEUE)
            if slave_ticket_key:
                a.append(slave_ticket_key)
        a = list(set(a))
        self.slave_tickets = a
        return self.slave_tickets


def iterate_tickets():
    found_today = 0
    list_closed_tickets = get_list_closed_tickets_from_master()
    for mt in list_closed_tickets:
        obj = Master(mt["key"], mt["resolution"], mt.get('links', []))
        obj.get_resolution()
        obj.get_slave_tickets()

        for st in obj.slave_tickets:
            transition_step(st, 'naStoroneSapporta')
            comment = 'Закрылся %s с резолюцией **%s**' %(str(obj.key), str(obj.resolution))
            post_comment(st, comment, SUMMONEES)
            write_logs(st + ' ' + comment)
            found_today += 1

    return found_today


def work():
    global log_comment
    log_comment = str('Начинаю призывать саппорт в тикеты ' + SLAVE_QUEUE + '...\n')
    found_today = iterate_tickets()
    write_logs('Сегодня нашлось %d тикетов' %(found_today))

work()
