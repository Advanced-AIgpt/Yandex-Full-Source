# -*- coding: utf-8 -*-

import requests
import xmltodict


class SERP(object):
    def __init__(self,
                 key,
                 user='kuptservol',
                 lr=213,
                 url="https://yandex.ru/search/xml",
                 sortby="rlv",
                 filter="strict",
                 maxpassages=1,
                 l10n="ru"):
        self.user = user
        self.lr = lr
        self.key = key
        self.url = url
        self.filter = filter
        self.sortby = sortby
        self.l10n = l10n
        self.maxpassages = maxpassages

    def search(self, query):
        response = requests.get(self.url,
                                params={'key': self.key,
                                        'user': self.user,
                                        'lr': self.lr,
                                        'l10n': self.l10n,
                                        'sortby': self.sortby,
                                        'filter': self.filter,
                                        'maxpassages': self.maxpassages,
                                        'query': query
                                        })

        response_as_dict = xmltodict.parse(response.content)


        results = response_as_dict.get('yandexsearch', {}).get('response', {}).get('results', {}).get('grouping', {}).get('group', [])

        return results
