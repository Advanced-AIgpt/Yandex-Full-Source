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
                            except:
                                print(ticket['key'], 'Another link deleted ')
                                print(event)
                except: print('Updated but not unlinked')

        if added or deleted:
            newDuplicatesCount = duplicatesCount + len(events) - len(deleted)
            print ('all duplicates', newDuplicatesCount)

            if newDuplicatesCount < 0:
                newDuplicatesCount = 0
                print (ticket['key'], ' now 0 links')
            print ('In ticket ', ticket['key'], ', count = ', duplicatesCount, ', added ', added, ', deleted ', deleted, ' now counter = ', newDuplicatesCount)
            data = json.dumps({'duplicatesCount': newDuplicatesCount})
            resp = requests.patch(STARTREK_API_URL + ticket['key'], data=data, headers=STARTREK_ROBOT_HEADER)
    return

change_counter_by_history('BILLINGSUP')
change_counter_by_history('SMARTTVSUP')
change_counter_by_history('LAUNCHERSUP')

