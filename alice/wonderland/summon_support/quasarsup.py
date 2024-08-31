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
SLAVE_QUEUE = 'QUASARSUP'

POSTFIX = '\n Закрылся один или несколько связанных тикетов ALICE.'
COMMENTS = {
    "fixed"     : 'Нам удалось справиться с задачей! Cкорее обрадуй пользователей!' + POSTFIX,
    "won'tFix"  : 'Разработчики сказали, что так и должно быть. :( увы.' + POSTFIX,
    "invalid"   : 'Признан некорректным. Ну, и такое бывает' + POSTFIX,
    "no_summon" : 'Призвать некого. Что мне делать?' + POSTFIX
}

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

def get_support_group():
    support_group = []
    resp = requests.get('https://st-api.yandex-team.ru/v2/users/?group=110862',
                        headers=STARTREK_ROBOT_HEADER)
    for i in resp.json():
        support_group.append(i['login'])
    return support_group

def get_list_closed_tickets_from_master():
    page = 1
    page_list_len = -1
    list_closed_tickets = []
    while page_list_len != 0:
        resp = requests.get(STARTREK_API_URL + '?filter=queue:' + MASTER_QUEUE
                            + '&fields=resolution' # &filter=key:ALICE-3375
                            + '&filter=status:closed&filter=resolved:today()&expand=links&page=' # &filter=resolved:today()
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

def get_author_and_followers(ticket_key):
    assert isinstance(ticket_key, str)
    resp = requests.get(STARTREK_API_URL + '/' + ticket_key + '?fields=resolution,author,followers',
                        headers=STARTREK_ROBOT_HEADER)
    ticket_body = resp.json()
    people = []
    author = ticket_body['createdBy']['id']
    people.append(author)
    for i in ticket_body['followers']:
        people.append(i['id'])
    return people

def get_summonees(ticket_key):
    assert isinstance(ticket_key, str)
    people = get_author_and_followers(ticket_key)
    support_group = get_support_group()
    summonees = list(set(people) & set(support_group))
    print(ticket_key, summonees)
    return summonees

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
            # write_logs('https://st.yandex-team.ru/' + st)
            transition_step(st, 'waitingForFeedback')
            comment = COMMENTS.get(obj.resolution, POSTFIX)
            summonees = get_summonees(st)
            if not summonees: comment = COMMENTS["no_summon"]
            post_comment(st, comment, summonees)
            found_today += 1

    return found_today


def work():
    global log_comment
    log_comment = str('Начинаю призывать саппорт в тикеты ' + SLAVE_QUEUE + '...\n')
    found_today = iterate_tickets()
    write_logs('Сегодня нашлось %d тикетов' %(found_today))
    post_yamb_message(log_comment)

work()
