import json
import logging

import requests
from dataclasses import dataclass

SOLOMON_API_URI = 'https://solomon.yandex.net/api/v2'


@dataclass(init=True)
class Alert:
    id: str
    version: int


def request(token: str, uri_suffix: str, method: str, params: dict = None, data: dict = None) -> dict:
    headers = {
        'Authorization': 'OAuth ' + token,
        'Accept': 'application/json',
        'Content-Type': 'application/json'
    }

    response = requests.request(method, f'{SOLOMON_API_URI}/{uri_suffix}', headers=headers, params=params,
                                json=data)

    response.raise_for_status()

    return json.loads(response.content)


def get_alert(token: str, project: str, alert_id: str) -> Alert:
    response = request(token, f'projects/{project}/alerts/{alert_id}', 'GET')
    return Alert(response['id'], response['version'])


def get_alert_ids(token: str, project: str) -> list[str]:
    next_page_token = None
    alert_ids = []
    while True:
        params = {
            'pageSize': 100
        }
        if next_page_token is not None:
            params['nextPageToken'] = next_page_token
        response = request(token, f'projects/{project}/alerts', 'GET', params)
        next_page_token = response.get('nextPageToken')
        for alert in response['items']:
            alert_ids.append(alert['id'])

        if next_page_token is None:
            break

    return alert_ids


def create_alert(token: str, project: str, alert: dict):
    request(token, f'projects/{project}/alerts', 'POST', data=alert)


def update_alert(token: str, project: str, alert: dict):
    alert_id = alert['id']
    alert_with_version = get_alert(token, project, alert_id)
    alert['version'] = alert_with_version.version
    request(token, f'projects/{project}/alerts/{alert_id}', 'PUT', data=alert)


def sync_alerts(token: str, project: str, alerts: list[dict]):
    alert_ids = set(get_alert_ids(token, project))

    for alert in alerts:
        if alert['id'] in alert_ids:
            logging.info(f'Updating {alert["id"]}...')
            update_alert(token, project, alert)
            logging.info(f'alert {alert["id"]} has been successfully updated')
        else:
            logging.info(f'Creating {alert["id"]}...')
            create_alert(token, project, alert)
            logging.info(f'Alert {alert["id"]} has been successfully created')
