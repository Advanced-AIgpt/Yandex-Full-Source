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


def post_yamb_message(msg):
    message = {"chat_id": YAMB_CHAT_ID, "text": msg}
    print(message)
    resp = requests.post( YAMB_API_URL + '/sendMessage/',
                        data=message,
                        headers=YAMB_HEADER)
    try:
        return resp.json()['message']
    except KeyError:
        print('Не удалось отправить комментарий\n' + str(resp.json()))
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

def get_weight(ticket):
    severity_value = 1 # суровость
    if 'severity' in ticket:
        severity = ticket['severity']['key']
        if severity == 'blocker': severity_value = 1
        if severity == 'critical': severity_value = 0.7
        if severity == 'normal': severity_value = 0.5
        if severity == 'minor': severity_value = 0.3
        if severity == 'trivial': severity_value = 0.1
    else: severity = '-'
    scope_value = 1 # массовость
    if 'scope' in ticket:
        scope = ticket['scope']
        if scope == 'few users': scope_value = 0.2
        if scope == 'dozens': scope_value = 0.4
        if scope == 'hundreds': scope_value = 0.6
        if scope == 'thousands': scope_value = 0.8
        if scope == 'all or majority': scope_value = 1
        if scope == u'единицы пользователей': scope_value = 0.2
        if scope == u'десятки': scope_value = 0.4
        if scope == u'сотни': scope_value = 0.6
        if scope == u'тысячи': scope_value = 0.8
        if scope == u'все или большинство': scope_value = 1
    else: scope = '-'
    reput_value = 1 # репутационные риски
    if 'reputationalRisks' in ticket:
        reput = ticket['reputationalRisks']
        if reput == 'No': reput_value = 1
        if reput == 'Yes': reput_value = 1.5
        if reput == u'нет': reput_value = 1
        if reput == u'да': reput_value = 1.5
    else: reput = '-'
    surface_value = 1 # поверхность
    if 'aliceSurfaces' in ticket:
        surface = ticket['aliceSurfaces']
        if len(surface) == 1:
            if 'watch elari' in surface: surface_value = 0.5
    else: surface = '-'
    current_stage_value = 1 # текущая стадия
    if 'currentStage' in ticket:
        current_stage = ticket['currentStage']
        if current_stage == 'Development' or current_stage == 'Testing':
            current_stage_value = 0.1
    else: current_stage = '-'
    duplicates_value = 1 # дубликаты
    if 'duplicatesCount' in ticket:
        duplicates = ticket['duplicatesCount']
        if duplicates > 0: duplicates_value = 1.1
        if duplicates > 10: duplicates_value = 1.5
    else: duplicates = 0

    print (severity, severity_value, scope, scope_value, reput, reput_value, surface, surface_value, current_stage, current_stage_value, duplicates, duplicates_value)
    weight_new = surface_value * current_stage_value * severity_value * scope_value * reput_value * duplicates_value * 100
    #print (weight_new)
    return weight_new


def change_weight():
    tickets_list = get_updated_tickets('ALICE')
    if not tickets_list:
            print('no new updated tickets')
            return
    for ticket in tickets_list:
        print(ticket['key'] + ' changed somehow')
        if ticket['type']['key'] == 'bug' and 'severity' in ticket:
            weight_new = round(get_weight(ticket),1)
            if 'weight' in ticket:
                weight_old = ticket['weight']
                if weight_old != weight_new:
                    data = json.dumps({'weight': weight_new})
                    resp = requests.patch(STARTREK_API_URL + ticket['key'], data=data, headers=STARTREK_ROBOT_HEADER)
                    print(weight_old, weight_new)
                else:
                    print('didnt change')
            else:
                data = json.dumps({'weight': weight_new})
                resp = requests.patch(STARTREK_API_URL + ticket['key'], data=data, headers=STARTREK_ROBOT_HEADER)
                print(weight_new)
    return

change_weight()
