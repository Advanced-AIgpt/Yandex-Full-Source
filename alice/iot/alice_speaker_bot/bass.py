# -*- coding: utf-8 -*-
import json

from urllib.parse import urljoin

import requests

BassService = 'quasar'
IoTClientID = 'ru.yandex.quasar.iot'
VideoClientID = 'ru.yandex.video'


class BassClient:
    def __init__(self, url):
        self.url = url

    def do(self, message, device_id, user_id):
        data = dict(
            service=BassService,
            event='text_action',
            service_data=dict(text_actions=[message]),
            callback_data=json.dumps(dict(did=device_id, uid=user_id, client_id=IoTClientID))
        )
        self._push(data)

    def say(self, message, device_id, user_id):
        data = dict(
            service=BassService,
            event='phrase_action',
            service_data=dict(phrase=message),
            callback_data=json.dumps(dict(did=device_id, uid=user_id, client_id=IoTClientID))
        )
        self._push(data)

    def youtube(self, youtube_video_id, timestamp, device_id, user_id):
        data = dict(
            service=BassService,
            event='play_video',
            service_data=dict(
                play_uri='https://www.youtube.com/watch?v={}&t={}s'.format(youtube_video_id, timestamp),
                provider_name='youtube',
                provider_item_id=youtube_video_id
            ),
            callback_data=json.dumps(dict(did=device_id, uid=user_id, client_id=VideoClientID))
        )
        self._push(data)

    def _push(self, data):
        response = requests.post(url=urljoin(self.url, 'push'),
                                 json=data)
        if response.status_code != 200:
            print('bad response')
            print(response.status_code)
            print(response.text)
            return None
