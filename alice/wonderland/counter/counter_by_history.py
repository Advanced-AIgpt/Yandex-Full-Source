#-*- coding: utf-8 -*-

import json
import os
import requests
from datetime import datetime, timedelta


STARTREK_API_URL = 'https://st-api.yandex-team.ru/v2/issues/'
STARTREK_ROBOT_TOKEN = os.getenv("STARTREK_OAUTH_TOKEN")
STARTREK_ROBOT_HEADER = {'Authorization': 'OAuth %s' % (STARTREK_ROBOT_TOKEN)}

YAMB_API_URL = 'https://bp.mssngr.yandex.net/bot'
YAMB_TOKEN = os.getenv("YAMB_TOKEN")
YAMB_CHAT_ID = os.getenv("YAMB_CHAT_ID_OSENNIKOVAK")
YAMB_HEADER = {'Authorization': 'OAuthTeam %s' % (YAMB_TOKEN)}

SAMSARA_API_URL = 'https://samsara.yandex-team.ru/api/v2/'
SAMSARA_ROBOT_TOKEN = os.getenv("SAMSARA_TOKEN")
SAMSARA_ROBOT_HEADER = {'Authorization': 'OAuth %s' % (SAMSARA_ROBOT_TOKEN)}


def post_yamb_message(msg):
    message = {"chat_id": YAMB_CHAT_ID, "text": msg}
    print(message)
    resp = requests.post( YAMB_API_URL + '/sendMessage/',
                        data=message,
                        headers=YAMB_HEADER)
    try:
        return resp.json()['message']
    except KeyError:
        print('Cant send msg\n' + str(resp.json()))
    print(resp.json())

'''
# получаем список последних обновлённых тикетов
def get_updated_tickets(queue):
    resp = requests.get(STARTREK_API_URL + '?query=Queue%3A%20'+ queue + '%20AND%20Updated%3A%20>%3D%20today()%20-%201d%20AND%20Type%3A%20bug%20', headers=STARTREK_ROBOT_HEADER)
    resp = resp.json()
    return resp
'''

def get_updated_tickets(queue):
    page = 1
    page_list_len = -1
    list_tickets = []
    while page_list_len != 0:
        resp = requests.get(STARTREK_API_URL + '?filter=queue:' + queue + '&filter=updated:today()&page=' + str(page), headers=STARTREK_ROBOT_HEADER)
        page_list_len = len(resp.json())
        if resp.json():
            list_tickets.extend(resp.json())
        print(page, page_list_len)
        page += 1
    return list_tickets

def get_open_tickets_in_ALICESUP(queue):
    page = 1
    page_list_len = -1
    list_tickets = []
    while page_list_len != 0:
        resp = requests.get(STARTREK_API_URL + '?filter=queue:' + queue + '&filter=resolution:empty()&&filter=type:incident&page=' + str(page), headers=STARTREK_ROBOT_HEADER)
        page_list_len = len(resp.json())
        if resp.json():
            list_tickets.extend(resp.json())
        print(page, page_list_len)
        page += 1
    return list_tickets

# получаем связи тикета со стартреком
def get_st_links(ticket):
    resp = requests.get(STARTREK_API_URL + ticket + '/links', headers=STARTREK_ROBOT_HEADER)
    resp = resp.json()
    rels_list = []
    for rels in resp:
        rels_list.append(rels['object']['key'])
    return rels_list

# список ссылок на внешние сервисы
def get_remote_links(ticket):
    resp = requests.get(STARTREK_API_URL + ticket + '/remotelinks', headers=STARTREK_ROBOT_HEADER)
    resp = resp.json()
    return resp

def is_yesterday(event):
    gmt = datetime.now() - timedelta(hours=3)
    return str(event['updatedAt'][:10]) == str((gmt - timedelta(days=1)).strftime('%Y-%m-%d'))

def is_today(event):
    gmt = datetime.now() - timedelta(hours=3)
    return str(event['updatedAt'][:10]) == str(gmt.strftime('%Y-%m-%d'))

def get_history(key):
    history = requests.get(STARTREK_API_URL + key + '/changelog', headers=STARTREK_ROBOT_HEADER)
    history_list = history.json()
    if len(history_list)<50:
        return history_list
    else:
        history_list = []
        for event in history.json():
            if is_today(event): history_list.append(event)
        header = history.headers['Link']
        start = header.find('rel="first", <') + 14
        end = header.find('>; rel="next"')
        next_event = header[start:end]
        #print (next_event)
        prev_event = ' '

        while prev_event != next_event:
            add_history = requests.get(next_event, headers=STARTREK_ROBOT_HEADER)
            prev_event = next_event
            header = add_history.headers['Link']
            end = header.find('>; rel="next"')
            if end == -1:
                #print(len(history_list))
                return history_list
            else:
                start = header.find('rel="first", <') + 14
                next_event = header[start:end]
                #print (next_event)
                for event in add_history.json():
                    if is_today(event): history_list.append(event)


def get_rels_from_support(alice, field):
    resp = requests.get(STARTREK_API_URL + alice + '/links', headers=STARTREK_ROBOT_HEADER)
    resp = resp.json()
    rels_list = []
    for rels in resp:
        rels_list.append(rels['object']['key'])
    print ('all links in ALICE ', rels_list)
    sup_list = []
    for rels in rels_list:
        if ('ALICESUP-' in rels) or ('QUASARSUP-' in rels):
            sup_list.append(rels)
    print('links from support ', sup_list)
    #забираем из них счетчики
    sum_count = 0
    for elem in sup_list:
        ticket = requests.get(STARTREK_API_URL + elem, headers=STARTREK_ROBOT_HEADER)
        ticket = ticket.json()
        if field in ticket:
            elem_count = ticket[field]
        else:
            elem_count = 0
        print ('in ', elem, elem_count)
        sum_count += elem_count
    return sum_count



def copy_counter_to_alice(ticket, field, value):
    key = ticket['key']
    print('---------------------------------')
    print('here is ', key)
    print('started to copy ', field)
    resp = requests.get(STARTREK_API_URL + key + '/links', headers=STARTREK_ROBOT_HEADER)
    resp = resp.json()
    rels_list = []
    alice_list = []
    alice_closed_list = []
    alice_open_list = []
    for rels in resp:
        rels_list.append(rels['object']['key'])
        if 'ALICE-' in rels['object']['key']:
            alice_list.append(rels['object']['key'])
            if rels['status']['key'] == "closed":
                alice_closed_list.append(rels['object']['key'])
            else:
                alice_open_list.append(rels['object']['key'])
    print ('all rels ', rels_list)
    print ('all alice list ', alice_list)
    print ('opened ', alice_open_list)
    print ('closed ', alice_closed_list)

    #print (rels_list)

    # давайте условно считать, что больше 1, а не больше нуля, так как могут сначала создать тикет, а потом прикрепить самсару
    if (len(rels_list) == 0) and value > 1:
        if field == 'duplicatesCount': post_yamb_message('in ' + key + ' no ALICE, but couner is ' + str(value))
        return

    if (len(rels_list) == 1) and (len(alice_list) == 1):
        alice = alice_list[0]
        print ('in ', key, 'related ', alice, 'copy to it')
        if alice in alice_closed_list:
            if field == 'duplicatesCount':
                if ticket['type']['key'] == 'bug' and ticket['status']['key'] == 'inProgress':
                    #post_yamb_message ('in ' + key + ' related ' + alice + ' is closed, but new duplicates are appeared')
                    new_text = 'Этот тикет "В работе" и обращения добавляются, но у него нет ни одного открытого связанного ALICE тикета. Тикет переоткрываю.'
                    data = json.dumps({'text':new_text})
                    resp = requests.post(STARTREK_API_URL + ticket['key'] + '/comments', data=data, headers=STARTREK_ROBOT_HEADER)
                    resp = requests.post(STARTREK_API_URL + ticket['key'] + '/transitions/open/_execute',  headers=STARTREK_ROBOT_HEADER)
                    print(resp, resp.text)
        #print ('all related to ', alice)
        newDuplicatesCount = get_rels_from_support(alice, field)
        if newDuplicatesCount !=0:
            print (newDuplicatesCount)
            data = json.dumps({field: newDuplicatesCount})
            resp = requests.patch(STARTREK_API_URL + alice, data=data, headers=STARTREK_ROBOT_HEADER)
        return

    if (len(rels_list) == 1) and (len(alice_list) == 0):
        if field == 'duplicatesCount':
            if ticket['type']['key'] == 'bug' and ticket['status']['key'] == 'inProgress':
                #post_yamb_message ('in ' + key + ' related ' + rels_list[0] + ' from wrong queue')
                new_text = 'Этот тикет "В работе" и обращения добавляются, но у него нет ни одного открытого связанного ALICE тикета. Тикет переоткрываю.'
                data = json.dumps({'text':new_text})
                resp = requests.post(STARTREK_API_URL + ticket['key'] + '/comments', data=data, headers=STARTREK_ROBOT_HEADER)
                resp = requests.post(STARTREK_API_URL + ticket['key'] + '/transitions/open/_execute',  headers=STARTREK_ROBOT_HEADER)
                print(resp, resp.text)
        return

    if (len(rels_list) > 1):
        alice_count = len(alice_open_list) + len(alice_closed_list)

        if alice_count == 0:
            if field == 'duplicatesCount':
                if ticket['type']['key'] == 'bug' and ticket['status']['key'] == 'inProgress':
                    #post_yamb_message ('in ' + key + ' all tickets from wrong queue')
                    new_text = 'Этот тикет "В работе" и обращения добавляются, но у него нет ни одного открытого связанного ALICE тикета. Тикет переоткрываю.'
                    data = json.dumps({'text':new_text})
                    resp = requests.post(STARTREK_API_URL + ticket['key'] + '/comments', data=data, headers=STARTREK_ROBOT_HEADER)
                    resp = requests.post(STARTREK_API_URL + ticket['key'] + '/transitions/open/_execute',  headers=STARTREK_ROBOT_HEADER)
                    print(resp, resp.text)

        if alice_count == 1:
            alice = alice_list[0]
            print ('in ', key, 'just one related ', alice, 'copy to it')
            if alice in alice_closed_list:
                if field == 'duplicatesCount':
                    if ticket['type']['key'] == 'bug' and ticket['status']['key'] == 'inProgress':
                        #post_yamb_message ('in ' + key + ' related ' + alice + ' is closed but duplicates are appeared')
                        new_text = 'Этот тикет "В работе" и обращения добавляются, но у него нет ни одного открытого связанного ALICE тикета. Тикет переоткрываю.'
                        data = json.dumps({'text':new_text})
                        resp = requests.post(STARTREK_API_URL + ticket['key'] + '/comments', data=data, headers=STARTREK_ROBOT_HEADER)
                        resp = requests.post(STARTREK_API_URL + ticket['key'] + '/transitions/open/_execute',  headers=STARTREK_ROBOT_HEADER)
                        print(resp, resp.text)
            #print ('all related ', alice)
            newDuplicatesCount = get_rels_from_support(alice, field)
            if newDuplicatesCount !=0:
                print (newDuplicatesCount)
                data = json.dumps({field: newDuplicatesCount})
                resp = requests.patch(STARTREK_API_URL + alice, data=data, headers=STARTREK_ROBOT_HEADER)
        if alice_count > 1:
            print ('in ' + key + ' more then one ALICE')
            if len(alice_open_list) == 0:
                if field == 'duplicatesCount':
                    if ticket['type']['key'] == 'bug' and ticket['status']['key'] == 'inProgress':
                        #post_yamb_message ('in ' + key + ' all from ALICE are closed but duplicates are appeared')
                        new_text = 'Этот тикет "В работе" и обращения добавляются, но у него нет ни одного открытого связанного ALICE тикета. Тикет переоткрываю.'
                        data = json.dumps({'text':new_text})
                        resp = requests.post(STARTREK_API_URL + ticket['key'] + '/comments', data=data, headers=STARTREK_ROBOT_HEADER)
                        resp = requests.post(STARTREK_API_URL + ticket['key'] + '/transitions/open/_execute',  headers=STARTREK_ROBOT_HEADER)
                        print(resp, resp.text)
            else:
                for elem in alice_open_list:
                    newDuplicatesCount = get_rels_from_support(elem, field)
                    print (newDuplicatesCount)
                    if newDuplicatesCount !=0:
                        data = json.dumps({field: newDuplicatesCount})
                        resp = requests.patch(STARTREK_API_URL + elem, data=data, headers=STARTREK_ROBOT_HEADER)
        return



def change_counter_by_history(queue):
    tickets_list = get_updated_tickets(queue)

    if not tickets_list:
        print('no new updated tickets')
        return
    for ticket in tickets_list:

        print(ticket['key'] + ' changed somehow')

        if 'duplicatesCount' in ticket:
            duplicatesCount = ticket['duplicatesCount']
        else:
            duplicatesCount = 0

        #history = requests.get(STARTREK_API_URL + ticket['key'] + '/changelog', headers=STARTREK_ROBOT_HEADER)
        #history = history.json()

        history = get_history(ticket['key'])

        added = []
        deleted = []
        support = 0
        alice_added_deleted = 0

        events  = []

        for event in history:
            if is_today(event) and (event['type'] == 'IssueLinked' or event['type'] == 'IssueUpdated'):
                try:
                    for link in event['links']:
                        try:
                            if link['to']['object']['application']['type'] == 'ru.yandex.otrs':
                                # сохраняем сюда добавленные
                                added.append(link['to']['object']['key'])
                                if not (link['to']['object']['key'] in events):
                                    events.append(link['to']['object']['key'])
                        except:
                            try:
                                print(ticket['key'], 'Internal link added ', event['links'][0]['to']['object']['key'])
                                if 'ALICE-' in event['links'][0]['to']['object']['key']:
                                    alice_added_deleted = 1
                            except:
                                print(ticket['key'], 'Another link added ')
                            print(event)
                except: print('Updated but not linked')

            if is_today(event) and (event['type'] == 'IssueUnlinked' or event['type'] == 'IssueUpdated'):
                try:
                    for link in event['links']:
                        try:
                            if link['from']['object']['application']['type'] == 'ru.yandex.otrs':
                                # сохраняем сюда удалённые
                                deleted.append(link['from']['object']['key'])

                                if link['from']['object']['key'] in events:
                                    events.remove(link['from']['object']['key'])
                        except:
                            try:
                                print(ticket['key'], 'Internal link deleted ', event['links'][0]['to']['object']['key'])
                                if 'ALICE-' in event['links'][0]['to']['object']['key']:
                                    alice_added_deleted = 1
                            except:
                                print(ticket['key'], 'Another link deleted ')
                                print(event)
                except: print('Updated but not unlinked')
            #updated by support
            if event['type'] =='IssueUpdated' and 'fields' in event:
                if event['fields'][0]['field']['id'] == 'duplicatesCount' and event['updatedBy']['id'] != 'robot-mad-hatter':
                    print(event['fields'][0]['field']['id'], event['updatedBy']['id'])
                    support = 1

        samsara_closed = 0
        #теперь вычитаем закрытые
        if ticket['type']['key'] == 'bug':
            links = get_remote_links (ticket['key'])
            if links:
                for elem in links:
                    if 'object' in elem:
                        if 'application' in elem['object']:
                            if 'type' in elem['object']['application']:
                                if elem['object']['application']['type'] == 'ru.yandex.otrs':
                                    samsara_ticket = requests.get(SAMSARA_API_URL + 'tickets/' + elem['object']['id'], headers=SAMSARA_ROBOT_HEADER)
                                    if samsara_ticket.status_code == 200:
                                        samsara_ticket = samsara_ticket.json()
                                        #print(samsara_ticket)
                                        #print(elem['object']['id'], samsara_ticket['resolution'], samsara_ticket['status'])
                                        #if samsara_ticket['resolution'] in ['RESOLVED', 'REJECTED', 'DUPLICATE']:
                                        if samsara_ticket['status'] in ['ARCHIVED', 'CLOSED']:
                                            samsara_closed += 1
                                        #else: print('another status')
                                    else: print('status code', samsara_ticket.status_code, samsara_ticket)
                print ('closed in samsara', samsara_closed)

        if added or deleted or support or alice_added_deleted or samsara_closed:
            newDuplicatesCount = duplicatesCount + len(events) - len(deleted)
            print ('all duplicates', newDuplicatesCount)
            without = 0
            if ticket['type']['key'] == 'bug':
                print ('duplicates without closed in samsara', newDuplicatesCount - samsara_closed)
                #вот тут обновление поля numOfDuplicatesWithoutCloseReq в САПОРТНОМ тикете
                without = newDuplicatesCount - samsara_closed
                #может быть такое, что что-то не то добавляли/прибавляли руками сотрудники сапорта
                if without < 0:
                    without = 0
                data = json.dumps({'numOfDuplicatesWithoutCloseReq': without})
                resp = requests.patch(STARTREK_API_URL + ticket['key'], data=data, headers=STARTREK_ROBOT_HEADER)

            if newDuplicatesCount < 0:
                newDuplicatesCount = 0
                print (ticket['key'], ' now 0 links')
            print ('In ticket ', ticket['key'], ', count = ', duplicatesCount, ', added ', added, ', deleted ', deleted, ' now counter = ', newDuplicatesCount)
            data = json.dumps({'duplicatesCount': newDuplicatesCount})
            resp = requests.patch(STARTREK_API_URL + ticket['key'], data=data, headers=STARTREK_ROBOT_HEADER)

            # после проапдейта нужно перенести
            # во всех очередях (ALICESUP, QUASARSUP и тд свои типы тикетов, например, "инцидент" и "ошибка")
            try:
                #post_yamb_message ('Start copy couner to ALICE')
                #post_yamb_message (ticket['key'])
                if newDuplicatesCount != duplicatesCount or support or alice_added_deleted or samsara_closed:
                    if 'ALICESUP-' in ticket['key']:
                        if ticket['type']['key'] == 'bug':
                            copy_counter_to_alice(ticket, 'duplicatesCount', newDuplicatesCount)
                            copy_counter_to_alice(ticket, 'numOfDuplicatesWithoutCloseReq', without)
                    if 'QUASARSUP-' in ticket['key']:
                        if ticket['type']['key'] == 'bug':
                            copy_counter_to_alice(ticket, 'duplicatesCount', newDuplicatesCount)
                            copy_counter_to_alice(ticket, 'numOfDuplicatesWithoutCloseReq', without)
            except: print('Can not copy counter from support to alice')
    return

change_counter_by_history('QUASARSUP')
change_counter_by_history('ALICESUP')
