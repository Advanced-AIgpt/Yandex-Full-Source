# coding: utf-8

import os
from six.moves.urllib.parse import quote_plus

import pymongo

from alice.review_bot.ybot.core.state import BaseState, register_state


def get_db_connection(conf):
    pswd = quote_plus(os.environ[conf['password_env']])
    db_name = conf['database']
    url = 'mongodb://{user}:{password}@{hosts}/{db_name}'.format(
        user=conf['user'],
        password=pswd,
        hosts=conf['mongo_hosts'],
        db_name=db_name,
    )

    return pymongo.MongoClient(
        url,
        connect=False,
        socketTimeoutMS=400,
        connectTimeoutMS=100,
        serverSelectionTimeoutMS=200,
        maxPoolSize=5,
        read_preference=pymongo.ReadPreference.PRIMARY_PREFERRED,
    )[db_name]


@register_state('mongodb')
class MongoState(BaseState):
    def _setup(self, conf):
        self._db = get_db_connection(conf)['reviews']

    def set(self, chat_id, name, value):
        self._db.update_one({'_id': chat_id}, {'$set': {name: value}}, upsert=True)

    def get(self, chat_id, name, default=None):
        data = self._db.find_one({'_id': chat_id})
        if data is None:
            return default
        return data.get(name, default)

    def delete(self, chat_id, name):
        self._db.delete_one({'_id': chat_id})

    def pop(self, chat_id, name, default=None):
        data = self.find_one_and_delete({'_id': chat_id})
        if data is None:
            return default
        return data.get(name, default)

    def get_chat_ids(self):
        return [item['_id'] for item in self._db.find({}, projection=[])]
