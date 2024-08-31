#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import json
import requests

###########################

STARTREK_API_URL = 'https://st-api.yandex-team.ru/v2/issues'
STARTREK_ROBOT_TOKEN = os.getenv("STARTREK_OAUTH_TOKEN")
STARTREK_ROBOT_HEADER = {'Authorization': 'OAuth %s' % (STARTREK_ROBOT_TOKEN)}

YAMB_API_URL = 'https://bp.mssngr.yandex.net/bot'
YAMB_TOKEN = os.getenv("YAMB_TOKEN")
YAMB_CHAT_ID = os.getenv("YAMB_CHAT_ID")
YAMB_HEADER = {'Authorization': 'OAuthTeam %s' % (YAMB_TOKEN)}

KNOWN_COMPONENTS = [57889, 57888, 57886, 57887, 57885, 57858, 57890, 57891, 57892, 57893, 57894, 57895, 64386, 65123]

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
        print('Не удалось отправить комментарий в Q. Причина: ' + str(resp.json()))

def get_queue_components():
    list_components = []
    resp = requests.get('https://st-api.yandex-team.ru/v2/queues/ALICERELEASE/components',
                        headers=STARTREK_ROBOT_HEADER)
    for c in resp.json():
        list_components.append(c['id'])
    return list_components

def search_new_components(list_components):
    assert isinstance(list_components, list)
    result=list(set(KNOWN_COMPONENTS) ^ set(list_components))
    if result:
        write_logs('Завезли новых компонент ' + str(result) + '. Хотим мы их закрывать?\nПолный список на https://st.yandex-team.ru/ALICERELEASE/components')

def get_prod_release_tickets():
    list_tickets = []
    resp = requests.get(STARTREK_API_URL + '?query=Queue:"ALICERELEASE" AND Status:Production AND Tags:!rm_main_ticket AND Components:notEmpty() AND Components:!"Релиз флагов"',
                        headers=STARTREK_ROBOT_HEADER)
    return resp.json()

def search_latest_release_in_component(ticket_key, component):
    assert isinstance(ticket_key, str)
    assert isinstance(component, str)
    resp = requests.get(STARTREK_API_URL + '?fields=key&query=Queue:ALICERELEASE AND Status:Production AND Components:' + component +' "Sort By":key DESC',
                        headers=STARTREK_ROBOT_HEADER)
    prod_component_releases = resp.json()
    if prod_component_releases:
        print('Latest release in component is ' + prod_component_releases[0]['key'])
        if prod_component_releases[0]['key'] != ticket_key:
            write_logs('https://st.yandex-team.ru/' + ticket_key + ' is not last release and will be closed')
            close_release_ticket(ticket_key)
        else:
            print(ticket_key + ' latest, OK')

def close_release_ticket(ticket_key):
    assert isinstance(ticket_key, str)
    new_resolution = json.dumps({"resolution": 'fixed'}, ensure_ascii=False, default=str)
    resp = requests.post('%s/%s/transitions/close/_execute' % (STARTREK_API_URL, ticket_key),
                         data=new_resolution,
                         headers=STARTREK_ROBOT_HEADER)
    try:
        return resp.json()[0]['self']
    except KeyError:
        write_logs('Не удалось поменять тикет. Причина: ' + str(resp.json()))

def work():
    list_components = get_queue_components()
    search_new_components(list_components)
    prod_releases = get_prod_release_tickets()
    for r in prod_releases:
        ticket_key = str(r['key'])
        component = str(r['components'][0]['id'])
        print(ticket_key, component)
        search_latest_release_in_component(ticket_key, component)
    post_yamb_message(log_comment)

log_comment = ''
work()
