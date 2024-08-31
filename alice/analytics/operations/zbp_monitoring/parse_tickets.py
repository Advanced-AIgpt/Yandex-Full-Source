# -*-coding: utf8 -*-
import os
import re
import json
import requests
from datetime import datetime
import nirvana.job_context as nv

ctx = nv.context()
parameters = json.loads(ctx.get_parameters().get('kwargs', '{}'))
inputs = ctx.get_inputs()
outputs = ctx.get_outputs()


def convert_date(dt):
    try:
        if len(dt) == 8:
            frmt = '%d.%m.%y'
        elif len(dt) == 10 and '.' in dt:
            frmt = '%d.%m.%Y'
        else:
            frmt = '%Y-%m-%d'
        return datetime.strptime(dt, frmt).strftime('%Y-%m-%d')
    except:
        return None


def get_device_ids(desc):
    pattern = r'(?:\s|^){1,}[A-Za-z0-9]{20,32}(?:\s|$){1,}'
    ids = list(set([i.strip() for i in re.findall(pattern, desc)]))
    return ids


def get_uuids(desc):
    pattern = r'[A-Za-z0-9]{32}'
    ids = list(set([i.strip().lower() for i in re.findall(pattern, desc)]))
    return ids


def get_req_ids(desc):
    pattern = r'https:\/\/setrace\.(?:in\.)*yandex\-team\.ru\/(?:ui\/|)alice\/(?:tracePage|traceByPrimaryKey|trace)\/[a-z0-9\-]{36}'
    ids = list(set([i.split('/')[-1] for i in re.findall(pattern, desc)]))
    return ids


def get_dates(desc, created, setraced):
    pattern = r'(?:\s|^){1,}(\d{2}\.\d{2}\.\d{2,4}|\d{4}\-\d{2}\-\d{2})(?:\s|$){1,}'
    dates = [convert_date(i.split()[0].strip()) for i in re.findall(pattern, desc)]
    dates = list(set([created] + setraced + [dt for dt in dates if dt]))
    return dates


def get_dates_from_req_ids(req_ids):
    res = []
    for req_id in req_ids:
        headers = {"Authorization": "OAuth {}".format(os.getenv('SETRACE_TOKEN', ''))}
        url = 'https://setrace.yandex-team.ru/v1/search/{}/tracepageAlice'.format(req_id)
        resp = requests.get(url, headers=headers, verify=False)
        if resp.status_code == 200:
            try:
                res.append(datetime.fromtimestamp(resp.json()['status']['tree']['min_event_time']/1000000.0).strftime('%Y-%m-%d'))
            except:
                continue
    return res


def parse_tickets(st_data):
    parsed = []
    for it in st_data:
        res = {}
        desc = '\n'.join([it['description']] + [i['text'] for i in it.get('comments', [])])
        res['ticket'] = it['key']
        res['name'] = it['summary'].encode('utf-8')
        res['device_ids'] = get_device_ids(desc)
        res['uuids'] = get_uuids(desc)
        res['req_ids'] = get_req_ids(desc)
        res['dates'] = get_dates(desc, it['createdAt'][:10], get_dates_from_req_ids(res['req_ids']))
        res['desc'] = desc.encode('utf-8')
        parsed.append(res)
    return parsed


def main():

    with open(inputs.get("input1"), "r") as f:
        st_data = json.load(f)

    with open(outputs.get('output1'), 'w') as f:
        json.dump(parse_tickets(st_data), f, indent=4, ensure_ascii=False)


if __name__ == '__main__':
    main()
