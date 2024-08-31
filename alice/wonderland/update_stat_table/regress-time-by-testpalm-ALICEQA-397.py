#!/usr/bin/env python
# -*- coding: utf-8 -*-

import json
import itertools as it
from datetime import datetime, timedelta

import requests
from requests.adapters import HTTPAdapter
from requests.packages.urllib3.util.retry import Retry

import os
import sys
reload(sys)
sys.setdefaultencoding('utf-8')

###########################

ST_API_URL = 'https://st-api.yandex-team.ru/v2/issues'
ST_TOKEN = os.getenv("STARTREK_OAUTH_TOKEN")
ST_HEADERS = {'Authorization': 'OAuth %s' % (ST_TOKEN)}

CHARTS_TOKEN = os.getenv("CHARTS_TOKEN")
SF_API_URL = 'https://upload.stat.yandex-team.ru/'
PATH = 'Adhoc/cubovaya/Graphs/regress_time_by_testpalm'
SF_HEADERS = {'Authorization': 'OAuth %s' % (ST_TOKEN)}

FILTER = '(Queue: ALICERELEASE OR Queue: SEAREL AND Components: "! quasar" OR Queue: SEAREL AND Components: "! iot" OR Queue: POODYA OR Queue: VIDEOBASSREL) AND Updated:>=today()-90d'
GET_TICKETS = '?query=' + FILTER + '&fields=tags,key,createdAt,summary&expand=links'
# {
# 	'query' : FILTER,
# 	'fields': 'tags,key,createdAt,summary',
# 	'expand': 'links'
# }
DICTIONARY = 'alice_components_for_dash'

TESTPALM_URL = 'https://testpalm-api.yandex-team.ru/testrun/'
TESTPALM_OAUTH_TOKEN = os.getenv("TESTPALM_OAUTH_TOKEN")
TESTPALM_CONTENT_HEADERS = {
    'Authorization': 'OAuth %s' % (TESTPALM_OAUTH_TOKEN),
    "Content-Type": "application/json"
}

####################
# create json dict #
####################

def create_json_dict():
	ticket_list = get_startrek_data_with_pagination(GET_TICKETS)
	res = []
	tags_list = get_tags_list()
	for ticket_body in ticket_list:
		components = find_components(ticket_body, tags_list)
		for component in components:
			ticket_dict = create_base_dict(ticket_body)
			ticket_dict["component"] = component
			ticket_dict["regress_time"] = calculate_regress_time_by_testpalm(ticket_body)
			res.append(ticket_dict)
	res.sort(key = lambda n: n['fielddate'])
	res = list(filter(lambda a: a["regress_time"] != 0, res))
	return res


def get_startrek_data_with_pagination(params):
    page, next_url = 1, -1
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
	links_str = clean_string(links_str)
	links = links_str.split()

	rel_next = 'rel="next"'
	if rel_next in links:
		next_link_index = links.index(rel_next) - 1
		next_link = links[next_link_index]

	return next_link


def clean_string(links):
	links = links.replace('<', '')
	links = links.replace('>', '')
	links = links.replace(';', '')
	links = links.replace(',', '')
	return links


def find_components(body, tags_list):
    components = []
    if body['tags']:
        components = list( set(body['tags']) & set(tags_list) )
    return components

def get_tags_list():
    resp = requests.get('%s/dictionary/' % (SF_API_URL),
    	params={'name': DICTIONARY},
    	headers={'Authorization': 'OAuth %s' % (CHARTS_TOKEN)})
    tags_list = resp.json()['tags_list']
    return tags_list


def create_base_dict(body):
	key = str(body['key'])
	date = add_3_hours_to_UTC_time(body['createdAt'])
	date = date[:10]
	print('create_base_dict ' + key)

	body_dict = {
		"key": key,
		"summary": body['summary'],
		"fielddate" : date,
		}
	return body_dict


def add_3_hours_to_UTC_time(eventdate):
	eventdate = eventdate[:19]
	eventdate = eventdate.replace('T', ' ')
	eventdate = datetime.strptime(eventdate, "%Y-%m-%d %H:%M:%S")
	eventdate += timedelta(hours=3)
	return str(eventdate)


#################
# testpalm part #
#################


def calculate_regress_time_by_testpalm(body):
	regress_time = 0
	remotelinks = body.get('remotelinks', 0)
	if remotelinks:
		testrun_id_list = create_list_of_testruns(remotelinks)
		regress_time = summarize_execution_time_by_testruns(testrun_id_list)
		regress_time = convert_ms_to_hours(regress_time)
	return regress_time


def create_list_of_testruns(remotelinks):
    testrun_id_list = []
    for i in remotelinks:
        display = i['display']

        start = display.find('testrun/alice/')
        start_len = start + len('testrun/')
        end = display.find('(ru.yandex.testpalm)')

        if start == -1:
            start = display.find('testrun/paskills/')
            start_len = start + len('testrun/')
            end = display.find('(ru.yandex.testpalm)')

        if start == -1:
            start = display.find('testrun/aliceui/')
            start_len = start + len('testrun/')
            end = display.find('(ru.yandex.testpalm)')
        
        if start == -1:
            start = display.find('testrun/iot-ui/')
            start_len = start + len('testrun/')
            end = display.find('(ru.yandex.testpalm)')

        if start == -1:
            start = display.find('testrun/quasar-ui/')
            start_len = start + len('testrun/')
            end = display.find('(ru.yandex.testpalm)')

        if start != -1:
            run_id = display[start_len:end]
            testrun_id_list.append(run_id)
    return testrun_id_list


def summarize_execution_time_by_testruns(testrun_id_list):
	regress_time = 0
	for i in testrun_id_list:
		info = get_testrun_info(i)
		if info:
			testrun_time = calculate_testrun_time(info)
			regress_time += testrun_time
	return regress_time


def get_testrun_info(run_id):
	resp = requests_retry_session().get('%s/%s' % (TESTPALM_URL, run_id),
		params={'include': 'testGroups.testCases'},
		headers=TESTPALM_CONTENT_HEADERS)
	if resp.status_code == 200:
		return resp.json()


def calculate_testrun_time(info):
	cases = []
	for group in info['testGroups']:
		cases.extend(group['testCases'])
	durations = []
	for case in cases:
		duration = normalize_duration(case['duration'])
		durations.append(duration)
	testrun_time = sum(durations)
	return testrun_time


def normalize_duration(duration):
	one_hour_in_ms = 60 * 60 * 1000
	if duration >= one_hour_in_ms:
		duration = one_hour_in_ms
	return duration


def convert_ms_to_hours(ms_time):
	ms_time = float(ms_time) / 1000 / 3600
	hours_time = round(ms_time, 1)
	print('hours_time ', hours_time)
	return hours_time


def requests_retry_session(
    retries=3,
    backoff_factor=0.3,
    status_forcelist=(500, 502, 504),
    session=None,
):
    session = session or requests.Session()
    retry = Retry(
        total=retries,
        read=retries,
        connect=retries,
        backoff_factor=backoff_factor,
        status_forcelist=status_forcelist,
    )
    adapter = HTTPAdapter(max_retries=retry)
    session.mount('http://', adapter)
    session.mount('https://', adapter)
    return session


####################
# post to statface #
# ####################

def post_statface(res):
    assert isinstance(res, list)
    resp = requests.post('%s/_api/report/data/%s/' % (SF_API_URL, PATH),
                        params={'scale': 'daily', 'replace_mask': 'key'},
                        headers={'Authorization': 'OAuth %s' % (CHARTS_TOKEN)},
                        json={'data': res})
    print(resp.status_code, resp.json())
    try:
        return resp.json()['message']
    except KeyError:
        print('Не удалось загрузить отчёт\n' + str(resp.json()))

res = create_json_dict()
post_statface(res)
