import requests
import json
from math import log
from alice.boltalka.telegram_bot.lib.module import Module
from alice.boltalka.extsearch.query_basesearch.lib.main import QueryBasesearch
from alice.boltalka.telegram_bot.lib.cache import Cache

class NlgsearchHttpSource(Module):
    class Options:
        host = 'general-conversation-hamster.yappy.beta.yandex.ru'
        port = 80
        experiments = ''
        max_results = 50
        min_ratio = 1.0
        context_weight = 1.0
        ranker = 'catboost'
        extra_params = None
        deterministic = True

    def set_args(self, args):
        super().set_args(args)
        self.searcher = QueryBasesearch(
            self.host,
            self.port,
            self.experiments,
            self.max_results,
            self.min_ratio,
            self.context_weight,
            self.ranker,
            self.extra_params,
            max_retries=1,
            deterministic=self.deterministic
        )

    def get_candidates(self, args, context):
        self.set_args(args)
        return [dict(text=el[0], relevance=el[1], source=el[2]) for el in self.searcher.get_replies(context)]


class NlgsearchHttpSourceWithEntity(NlgsearchHttpSource):
    class Options:
        host = 'general-conversation-hamster.yappy.beta.yandex.ru'
        port = 80
        experiments = ''
        max_results = 50
        min_ratio = 1.0
        context_weight = 1.0
        ranker = 'catboost'
        extra_params = None
        deterministic = True
        entity_type = 'specific'
        entity_id = 2213

    def get_candidates(self, args, context):
        self.set_args(args)
        if self.entity_id is None:
            return []
        entity = f'{self.entity_type}:{self.entity_id}'
        extra_params = dict(ProtocolRequest=1, UseBaseModelsOnly=1, BaseKnnIndexName=entity, Entities=entity)
        if self.extra_params:
            print(self.extra_params)
            extra_params.update(self.extra_params)
        searcher = QueryBasesearch(
            self.host,
            self.port,
            self.experiments,
            self.max_results,
            self.min_ratio,
            self.context_weight,
            self.ranker,
            extra_params,
            deterministic=self.deterministic
        )
        replies = searcher.get_replies(context)
        print(replies)
        return [dict(text=el[0], relevance=el[1], source=el[2]) for el in replies]


class NlgsearchRanker(Module):
    class Options:
        weight = 1

    def get_scores(self, args, context, candidates):
        self.set_args(args)
        return [log(el['relevance'] * 1e-9 + 1e-9) if 'relevance' in el else -1e9 for el in candidates]
