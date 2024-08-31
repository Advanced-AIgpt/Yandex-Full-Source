from enum import Enum
import json
import requests

JUGGLER_EVENTS_URI = 'http://juggler-push.search.yandex.net/events'


class Status(Enum):
    OK = 1, 'OK'
    WARN = 2, 'WARN'
    CRIT = 3, 'CRIT'

    def worse(self, other):
        return self.value[0] > other.value[0]

    def __str__(self):
        return self.value[1]


def send_raw_event(host, service, status, tags, source, description=''):
    data = {
        'source': source,
        'events': [
            {
                'description': description,
                'host': host,
                'service': service,
                'status': status,
                'tags': tags
            }
        ]
    }
    response = requests.post(JUGGLER_EVENTS_URI, data=json.dumps(data))
    response.raise_for_status()
    response_json = json.loads(response.text)
    assert response_json['success']
    assert response_json['accepted_events'] == 1
    return response
