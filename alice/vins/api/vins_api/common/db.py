# coding: utf-8
import os
import pymongo
import ssl


def get_db_connection(settings):
    return pymongo.MongoClient(
        settings.MONGODB_URL,
        connect=False,
        socketTimeoutMS=400,
        connectTimeoutMS=100,
        serverSelectionTimeoutMS=200,
        maxPoolSize=5,
        read_preference=pymongo.ReadPreference.PRIMARY_PREFERRED,
    )[settings.MONGODB_NAME]


def get_db_connection_with_ssl(settings):
    return pymongo.MongoClient(
        settings.MONGODB_URL,
        connect=False,
        socketTimeoutMS=400,
        connectTimeoutMS=100,
        serverSelectionTimeoutMS=200,
        maxPoolSize=5,
        ssl_ca_certs=os.environ['VINS_YANDEX_ALL_CAS_PATH'],
        ssl_cert_reqs=ssl.CERT_REQUIRED,
        read_preference=pymongo.ReadPreference.PRIMARY_PREFERRED,
    )[settings.MONGODB_NAME]


def get_redis_connection():
    import redis
    if 'VINS_REDIS_PORT' in os.environ:
        port = int(os.environ['VINS_REDIS_PORT'])
    else:
        port = 6379
    return redis.StrictRedis(host='localhost', port=port, db=0)


def get_redis_knn_cache_connection():
    import redis
    if 'VINS_REDIS_KNN_CACHE_PORT' in os.environ:
        port = int(os.environ['VINS_REDIS_KNN_CACHE_PORT'])
    else:
        port = 6380
    return redis.StrictRedis(host='localhost', port=port, db=0, socket_timeout=0.03)
