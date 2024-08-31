#!/usr/bin/env python
# -*- coding: utf-8 -*-


import os
import json
import requests
import random

STARTREK_API_URL = 'https://st-api.yandex-team.ru/v2/issues'
STARTREK_ROBOT_TOKEN = os.getenv("STARTREK_OAUTH_TOKEN")
STARTREK_ROBOT_HEADER = {'Authorization': 'OAuth %s' % (STARTREK_ROBOT_TOKEN)}

YAMB_API_URL = 'https://bp.mssngr.yandex.net/bot'
YAMB_TOKEN = os.getenv("YAMB_TOKEN")
YAMB_CHAT_ID = os.getenv("YAMB_CHAT_ID")
YAMB_HEADER = {'Authorization': 'OAuthTeam %s' % (YAMB_TOKEN)}

WRONG_QUEUES_STRING = 'DIALOG,ASSISTANT'
RIGHT_QUEUE = 'ALICE'

TESTERS = ['osennikovak', 'cubovaya', 'kr1kaqt']


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

def get_bugs_from_wrong_queue():
    page = 1
    page_list_len = -1
    list_tickets = []
    while page_list_len != 0:
        resp = requests.get(STARTREK_API_URL + '?fields=resolution&query=Queue:' + WRONG_QUEUES_STRING + ' AND Type:Bug AND Created:>=28.05.2019&page=' + str(page),
                            headers=STARTREK_ROBOT_HEADER)
        page_list_len = len(resp.json())
        if resp.json():
            list_tickets.extend(resp.json())
        print(page, page_list_len)
        page += 1
    return list_tickets

def get_error_message(resp_json):
    assert isinstance(resp_json, dict)
    errorMessage = resp_json.get('errorMessages',[u'Неизвестна'])
    errorMessage = errorMessage[0].encode('utf8')
    return errorMessage

def try_move_ticket_to_queue(ticket_key):
    assert isinstance(ticket_key, str)
    resp = requests.post('%s/%s/_move?queue=%s' % (STARTREK_API_URL, ticket_key, RIGHT_QUEUE),
                         # data=new_resolution,
                         headers=STARTREK_ROBOT_HEADER)
    write_logs('https://st.yandex-team.ru/' + ticket_key + ' перемещён в очередь ' + RIGHT_QUEUE)
    resp_json = resp.json()
    moving_ticket_error = get_error_message(resp_json)
    try:
        return resp.json()['key']
    except KeyError:
        write_logs('Не удалось перенести тикет. Причина: ' + moving_ticket_error + '. Призвал тестировщиков.') #str(resp.json()))
        post_comment_with_summoner(ticket_key,moving_ticket_error)

def post_comment(new_ticket_key):
    assert isinstance(new_ticket_key, str)
    ticket_queue = new_ticket_key.split('-')[0]
    text = 'Для багов предназначена очередь ' + RIGHT_QUEUE + '. Если тикет должен находиться в другой очереди, укажи, пожалуйста, причину и верни на место.'
    comment = json.dumps({"text": text}, default=str)
    resp = requests.post('%s/%s/comments' % (STARTREK_API_URL, new_ticket_key),
                         data=comment,
                         headers=STARTREK_ROBOT_HEADER)
    resp_json = resp.json()
    post_comment_error = get_error_message(resp_json)
    try:
        return resp.json()['self']
    except KeyError:
        write_logs(str('Не удалось отправить комментарий в тикет. Причина: ') + post_comment_error) #str(resp.json()))

def post_comment_with_summoner(ticket_key,moving_ticket_error):
    assert isinstance(ticket_key, str)
    assert isinstance(moving_ticket_error, str)
    randnum = random.randrange(0, 3, 1)
    text = 'Не смог перенести тикет в очередь ' + RIGHT_QUEUE + '. Причина: ' + moving_ticket_error
    comment = json.dumps({"text": text, 'summonees': TESTERS[randnum]}, default=str)
    resp = requests.post('%s/%s/comments' % (STARTREK_API_URL, ticket_key),
                         data=comment,
                         headers=STARTREK_ROBOT_HEADER)
    resp_json = resp.json()
    post_comment_error = get_error_message(resp_json)
    try:
        return resp.json()['self']
    except KeyError:
        write_logs(str('Не удалось отправить комментарий с призывом. Причина: ') + post_comment_error) # + str(resp.json()))

def get_ticket_moves_changelog(ticket_key):
    assert isinstance(ticket_key, str)
    resp = requests.get(STARTREK_API_URL + '/' + ticket_key + '/changelog?type=IssueMoved',
                        headers=STARTREK_ROBOT_HEADER)
    return resp.json()

def search_movement_to_alice(changelog):
    assert isinstance(changelog, list)
    for log in changelog:
        queue_to_move = log['fields'][0]['to']['key']
        if queue_to_move == RIGHT_QUEUE:
            print('Не переносим баг; он уже был в ' + RIGHT_QUEUE)
            return 'movement found'

def check_movement_need(ticket_key):
    assert isinstance(ticket_key, str)
    changelog = get_ticket_moves_changelog(ticket_key)
    is_ticket_moved = 'movement not found'
    if changelog:
        is_ticket_moved = search_movement_to_alice(changelog)
    if is_ticket_moved == 'movement not found':
        print(is_ticket_moved)
        new_ticket_key = try_move_ticket_to_queue(ticket_key)
        if new_ticket_key:
            new_ticket_key = str(new_ticket_key)
            post_comment(new_ticket_key)

def work():
    list_bugs = get_bugs_from_wrong_queue()
    list_ticket_keys = []
    if list_bugs:
        for bug in list_bugs:
            ticket_key = str(bug.get('key'))
            print(ticket_key)
            check_movement_need(ticket_key)
        if log_comment:
            post_yamb_message(log_comment)

log_comment = ''
work()
