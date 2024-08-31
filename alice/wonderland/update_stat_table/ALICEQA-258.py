#!/usr/bin/env python
# -*- coding: utf-8 -*-

import json
import requests
import uuid
import os
import itertools as it
from datetime import datetime, timedelta

import sys
reload(sys)
sys.setdefaultencoding('utf-8')

###########################

ST_API_URL = 'https://st-api.yandex-team.ru/v2/issues'
STARTREK_OAUTH_TOKEN = os.getenv("STARTREK_OAUTH_TOKEN")
ST_HEADERS = {'Authorization': 'OAuth %s' % (STARTREK_OAUTH_TOKEN)}

CHARTS_TOKEN = os.getenv("CHARTS_TOKEN")
SF_API_URL = 'https://upload.stat.yandex-team.ru/_api'
PATH = 'Adhoc/cubovaya/Graphs/zbp-components'

FILTER = 'Queue: ALICERELEASE AND (Status: Production OR Resolution: Fixed) AND Updated:>=today()-3d'
DICTIONARY = 'alice_components_for_dash'

#####################
# create json value #
#####################

def get_tickets():
    page, page_list_len = 1, -1
    list_tickets = []

    while page_list_len != 0:
        resp = requests.get('%s?query=%s&fields=tags,components,key,summary,resolution&page=%d' \
            % (ST_API_URL, FILTER, page), headers=ST_HEADERS)

        page_list_len = len(resp.json())
        print(page, page_list_len)

        if resp.json(): list_tickets.extend(resp.json())
        page += 1
    return list_tickets


def get_changelog(key):
	assert isinstance(key, str)
	resp = requests.get('%s/%s/changelog?field=status' \
		% (ST_API_URL, key), headers=ST_HEADERS)
	changelog = resp.json()
	return changelog


def add_3_hours(eventdate):
	eventdate = eventdate[:19]
	eventdate = eventdate.replace('T', ' ')
	eventdate = datetime.strptime(eventdate, "%Y-%m-%d %H:%M:%S")
	eventdate += timedelta(hours=3)
	return str(eventdate)


def analyze_changelog(changelog):
	assert isinstance(changelog, list)
	for event in changelog:
		for i in event['fields']:
			if i['field']['id'] == 'status':
				endstatus = i['to']['key']
				date = add_3_hours(event['updatedAt']) # MSK timezone +3
				date = date[:10]
	return date


def create_ticket_dict(body):
	assert isinstance(body, dict)
	component = 0
	tags_list = get_tags_list()
	if body['tags']:
		component = ''.join([i for i in body['tags'] if i in tags_list])
	key = str(body['key'])
	changelog = get_changelog(key)
	date = analyze_changelog(changelog)
	row_uuid = str(uuid.uuid4())

	body_dict = {
		"uuid": row_uuid,
		"key": key,
		"summary": body['summary'],
		"component": component,
		"fielddate" : date
		}
	return body_dict


def get_tags_list():
    resp = requests.get('%s/dictionary/' % (SF_API_URL),
                        params={'name': DICTIONARY},
                        headers={'Authorization': 'OAuth %s' % (CHARTS_TOKEN)})
    tags_list = resp.json()['tags_list']
    return tags_list


list_tickets = get_tickets()
res = []
for ticket_body in list_tickets:
	ticket_dict = create_ticket_dict(ticket_body)
	if ticket_dict["component"]: res.append(ticket_dict)
res.sort(key = lambda n: n['fielddate'])

####################
# post to statface #
# ####################

def post_statface(res):
    assert isinstance(res, list)
    resp = requests.post('%s/report/data/%s/' % (SF_API_URL, PATH),
                        params={'scale': 'daily', 'replace_mask': 'fielddate'},
                        headers={'Authorization': 'OAuth %s' % (CHARTS_TOKEN)},
                        json={'data': res})
    print(resp.status_code, resp.json())
    try:
        return resp.json()['message']
    except KeyError:
        print('Не удалось загрузить отчёт\n' + str(resp.json()))

post_statface(res)
