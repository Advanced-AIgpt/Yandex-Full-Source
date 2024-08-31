#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import json
import uuid
import itertools as it
from datetime import datetime, timedelta
import requests

import sys
reload(sys)
sys.setdefaultencoding('utf-8')

###########################
# блок констант и токенов #
###########################

ST_API_URL = 'https://st-api.yandex-team.ru/v2/issues'
ST_TOKEN = os.getenv("STARTREK_OAUTH_TOKEN")
ST_HEADERS = {'Authorization': 'OAuth %s' % (ST_TOKEN)}

CHARTS_TOKEN = os.getenv("CHARTS_TOKEN")
SF_API_URL = 'https://upload.stat.yandex-team.ru/_api'
PATH = 'Adhoc/cubovaya/Graphs/Release_time_by_Startrek'
SF_HEADERS = {'Authorization': 'OAuth %s' % (ST_TOKEN)}

FILTER = 'Queue: ALICERELEASE AND (Status: Production OR Resolution: Fixed) AND Updated:>=today()-30d'
GET_TICKETS = '?query=' + FILTER + '&fields=tags,key,createdAt&expand=links'
# {
# 	'query' : FILTER,
# 	'fields': 'tags,key,createdAt,summary',
# 	'expand': 'links'
# }
GET_CHANGELOG = 'changelog?type=IssueWorkflow,IssueCreated,IssueCloned'
DICTIONARY = 'alice_components_for_dash'

##########################
# verify and update stat #
# ########################

def verify_stat_config():
	queue_statuses = get_queue_statuses()
	stat_config = get_stat_config()
	measures = select_measures(stat_config)
	absent_measures = find_absent_measures(queue_statuses, measures)
	if absent_measures:
		# TODO прикрутить в чартах ошибку на изменение кол-ва статусов?
		new_config = create_new_config(stat_config, absent_measures)
		update_stat_config(new_config)


def get_queue_statuses():
	resp = requests.get('https://st-api.yandex-team.ru/v2/queues/ALICERELEASE/statuses',
		headers=ST_HEADERS)
	queue_statuses = [i['key'].lower() for i in resp.json()]
	return queue_statuses


def get_stat_config():
	resp = requests.get('%s/report/config/%s/' % (SF_API_URL, PATH),
		headers={'Authorization': 'OAuth %s' % (CHARTS_TOKEN)})
	stat_config = resp.json()
	return stat_config


def select_measures(stat_config):
	assert isinstance(stat_config, dict)
	measures_dicts = stat_config['user_config']['measures']
	measures = []
	for i in measures_dicts:
		measures.extend(i.keys())
	return measures


def find_absent_measures(queue_statuses, measures):
	assert isinstance(queue_statuses, list)
	assert isinstance(measures, list)
	absent_measures= list(set(queue_statuses)-set(measures))
	absent_measures = list(map(str, absent_measures))
	return absent_measures


def create_new_config(stat_config, absent_measures):
	title = stat_config['title']
	user_config = stat_config['user_config']
	for i in absent_measures:
		user_config['measures'].append({i: 'string'})
	new_config = {'title': title, 'user_config': user_config}
	return new_config


def update_stat_config(new_config):
	resp = requests.post('%s/report/config/%s/' % (SF_API_URL, PATH),
           headers={'Authorization': 'OAuth %s' % (CHARTS_TOKEN)},
           json=new_config)
	try:
		return resp.json()['scale']
	except KeyError:
		print('Не удалось обновить отчёт\n' + resp.json())

########################
# collect data to stat #
# ######################

def collect_data_to_stat():
	ticket_list = get_startrek_data_with_pagination(GET_TICKETS)
	filtered_ticket_list = filter_tickets_by_components(ticket_list)
	data = create_dict(filtered_ticket_list)
	data.sort(key = lambda n: n['fielddate'])
	return data


def get_startrek_data_with_pagination(params):
    page = 1
    data = []
    next_link = '%s%s&page=%d' % (ST_API_URL, params, page)

    while next_link:
        resp = requests.get(next_link, headers=ST_HEADERS)
        data.extend(resp.json())
        next_link = find_next_link(resp.headers)

        page_list_len = len(resp.json())
        print(page, page_list_len)
        page += 1
    return data


def find_next_link(headers):
	next_link = False
	links_str = headers['Link']
	links_str = cleaning_string(links_str)
	links = links_str.split()

	rel_next = 'rel="next"'
	if rel_next in links:
		next_link_index = links.index(rel_next) - 1
		next_link = links[next_link_index]
	return next_link


def cleaning_string(links):
	links = links.replace('<', '')
	links = links.replace('>', '')
	links = links.replace(';', '')
	links = links.replace(',', '')
	return links


def filter_tickets_by_components(ticket_list):
	for body in ticket_list:
		body['component'] = find_components(body)
	filtered_ticket_list = list(filter( lambda x: x['component'], ticket_list ))
	return filtered_ticket_list


def find_components(body):
	component = 0
	tags_list = get_tags_list()
	if body['tags']:
		component = ''.join([i for i in body['tags'] if i in tags_list])
	return component


def get_tags_list():
    resp = requests.get('%s/dictionary/' % (SF_API_URL),
    	params={'name': DICTIONARY},
    	headers={'Authorization': 'OAuth %s' % (CHARTS_TOKEN)})
    tags_list = resp.json()['tags_list']
    return tags_list


def create_dict(ticket_list):
	data = []
	for body in ticket_list:

		key = body['key']
		print('Look at ' + key)

		changelog = get_changelog(key)
		statuses = get_status_durations_from_changelog(changelog)

		date = str(add_3_hours_to_UTC_time(body['createdAt']))
		date = date[:10]

		tktdct = {'key': key, "fielddate" : date, 'component': body['component']}
		tktdct.update(statuses)
		data.append(tktdct)

	return data


def get_changelog(key):
	params = '/%s/%s' % (key, GET_CHANGELOG)
	changelog = get_startrek_data_with_pagination(params)
	changelog = sort_changelog(changelog)
	return changelog

def sort_changelog(changelog):
	sorted_changelog = sorted(changelog, key=lambda x: x['updatedAt'])
	return sorted_changelog


# слишком длинная функция. подумать о переделке
def get_status_durations_from_changelog(changelog):
	assert isinstance(changelog, list)
	durations = create_statuses_dict()

	first_event = changelog[0]
	last_event = changelog[-1]

	for event in changelog:
		eventdate = add_3_hours_to_UTC_time(event['updatedAt'])

		if event is first_event:
			to_status = 'open'

		else:
			status_field = select_status_field(event['fields'])
			from_status = status_field['from']['key'].lower()
			to_status = status_field['to']['key'].lower()

			time_in_status = eventdate - prev_eventdate
			durations[from_status] += time_in_status
			# пребывание несколько раз в одном статусе

		prev_eventdate = eventdate

		if event is last_event:
			print(to_status, 'is last status')
			now = datetime.now()
			now = now.replace(microsecond=0)
			durations[to_status] += now - eventdate

	statuses_by_hours = {status: time_to_hours(time) for status, time in durations.items()}
	return(statuses_by_hours)


def create_statuses_dict():
	statuses = get_queue_statuses()
	durations = dict((status, timedelta(0)) for status in statuses)
	return durations


def select_status_field(fields):
	assert isinstance(fields, list)
	for i in fields:
		if i['field']['id'] == 'status':
			return i


def add_3_hours_to_UTC_time(eventdate):
	eventdate = eventdate[:19]
	eventdate = eventdate.replace('T', ' ')
	eventdate = datetime.strptime(eventdate, "%Y-%m-%d %H:%M:%S")
	eventdate += timedelta(hours=3)
	return eventdate


def time_to_hours(time):
	from_seconds = round(float(time.seconds)/3600, 1)
	hours = from_seconds + time.days*24
	return hours

####################
# post to statface #
####################

def post_statface(data):
    assert isinstance(data, list)
    resp = requests.post('%s/report/data/%s/' % (SF_API_URL, PATH),
                        params={'scale': 'daily', 'replace_mask': 'key'},
                        headers={'Authorization': 'OAuth %s' % (CHARTS_TOKEN)},
                        json={'data': data})
    print(resp.status_code, resp.json())
    try:
        return resp.json()['message']
    except KeyError:
        print('Не удалось загрузить отчёт\n' + resp.json())


verify_stat_config()
data = collect_data_to_stat()
post_statface(data)
