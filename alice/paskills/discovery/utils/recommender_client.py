# -*- coding: utf-8 -*-

import requests


class Recommender(object):
    def __init__(self,
                 experiment='discovery_bass_search_1.0',
                 card_name='discovery_bass_search',
                 min_word_num=1,
                 url="http://skills-rec-test.alice.yandex.net/recommender",
                 client_id="com.yandex.browser/19.7.7.115+TECNO;android+8.1.0%29",
                 uuid="65b937cc-8897-4277-b16b-eac8ce63eb1d"):
        self.experiment = experiment
        self.min_word_num = min_word_num
        self.card_name = card_name
        self.url = url
        self.client_id = client_id
        self.uuid = uuid

    def search(self, query):
        response = requests.get(self.url,
                                params={'card_name': self.card_name,
                                        'client_id': self.client_id,
                                        'experiment': self.experiment,
                                        'uuid': self.uuid,
                                        'utterance': query
                                        })

        response_json = response.json()

        return response_json
