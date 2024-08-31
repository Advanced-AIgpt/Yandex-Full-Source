import os
import json
import re
import collections.abc

from distutils.util import strtobool
from frozendict import frozendict

from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.settings.spotter_maps import SpotterMaps


logger = Logger.get('uniproxy.settings')


QLOUD_ENVIRONMENT = os.environ.get('QLOUD_ENVIRONMENT')
QLOUD_COMPONENT = os.environ.get('QLOUD_COMPONENT')
QLOUD_DISCOVERY_INSTANCE = os.environ.get('QLOUD_DISCOVERY_INSTANCE')
IS_QLOUD = os.environ.get('IS_QLOUD')
DO_NOT_LOG_EVENTS = os.environ.get('DO_NOT_LOG_EVENTS', '')

UNIPROXY_MAX_FORKS = os.environ.get('UNIPROXY_MAX_FORKS', 0)

DISABLE_SESSION_LOG = os.environ.get('DISABLE_SESSION_LOG')

UNIPROXY_INSTALLATION = 'uniproxy-qloud' if IS_QLOUD else 'uniproxy-rtc'

# this settings_dir accords to `alice/uniproxy/library/settings`. not to real path
settings_dir = os.path.dirname(__file__)
# real path of running app
run_dir = os.getcwd()

CUSTOM_SETTINGS_DIR = '.' if os.environ.get('CUSTOM_SETTINGS_DIR') else None

environment = None


def try_load_experiments(config):
    """ Try to load experiment configs and macros from local files and set next variables in `config`:
    - `experiments/list` - list of experiment configs
    - `experiments/macros` - list of experiment macros
    """

    MACROS_FILE_NAME = "vins_experiments.json"

    exp_dir = "/opt/experiments"
    if os.path.isdir(exp_dir):
        subdirs = [d for d in os.listdir(exp_dir) if os.path.isdir(os.path.join(exp_dir, d))]
        if subdirs and os.path.isfile(os.path.join(exp_dir, subdirs[0], MACROS_FILE_NAME)):
            exp_dir = os.path.join(exp_dir, subdirs[0])
        else:
            exp_dir = os.path.join(settings_dir, "experiments")
    else:
        exp_dir = os.path.join(run_dir, "experiments")

    logger.error(f'load exps from {exp_dir}')

    if "ycloud" in environment:
        exps_path = os.path.join(exp_dir, "experiments_ycloud.json")
    else:
        exps_path = os.path.join(exp_dir, "experiments_rtc_production.json")
    macros_path = os.path.join(exp_dir, MACROS_FILE_NAME)

    if not os.path.isfile(exps_path):
        return False

    with open(exps_path) as f:
        config["experiments"]["list"] = json.load(f)
    if os.path.isfile(macros_path):
        with open(macros_path) as f:
            config["experiments"]["macros"] = json.load(f)
    return True


def get_environment_type():
    env_type = os.environ.get('UNIPROXY_CUSTOM_ENVIRONMENT_TYPE')

    if not env_type:
        env_type = QLOUD_ENVIRONMENT

    if not env_type:
        try:
            with open('/etc/yandex/environment.type', 'r') as env:
                env_type = env.read().rstrip()
        except IOError:
            env_type = 'development'

    return env_type


def load_json_file(path):
    if CUSTOM_SETTINGS_DIR:
        if os.path.exists(os.path.join(CUSTOM_SETTINGS_DIR, path)):
            with open(os.path.join(CUSTOM_SETTINGS_DIR, path)) as fin:
                config = json.load(fin)
                return config

    if os.path.exists(os.path.join(settings_dir, path)):
        with open(os.path.join(settings_dir, path)) as fin:
            config = json.load(fin)
    else:
        from library.python import resource
        data = resource.find('/' + path)
        config = json.loads(data)

    return config


def load_settings(env_type):
    return load_json_file(env_type + '.json')


def load_tts_settings(env_type):
    return load_json_file('tts_' + env_type + '.json')


class frozenishdict(frozendict):
    def set_by_path(self, path: str, value):
        def set_dict_val_by_path(d: collections.abc.Mapping, path: str, value):
            keys = path.split(".", 1)
            if len(keys) == 2 and keys[0] in path:
                next_dict = d[keys[0]]
                if isinstance(next_dict, frozenishdict):
                    next_dict.set_by_path(keys[1], value)
                elif isinstance(next_dict, collections.abc.Mapping):
                    set_dict_val_by_path(next_dict, keys[1], value)
            elif keys:
                if isinstance(d, frozenishdict):
                    d._dict[keys[0]] = value
                elif isinstance(d, collections.abc.Mapping):
                    d[keys[0]] = value
        set_dict_val_by_path(self._dict, path, value)


env_type = get_environment_type()
if env_type in ['development']:
    environment = 'development'
elif env_type in ['rtc_production', 'production']:
    environment = 'rtc_production'
elif env_type in ['rtc_delivery_production']:
    environment = 'rtc_delivery_production'
elif env_type in ['rtc_alpha']:
    environment = 'rtc_alpha'
elif env_type in ['testing', 'test']:
    environment = 'testing'
elif env_type in ['rtc_delivery_alpha']:
    environment = 'rtc_delivery_alpha'
elif env_type in ['ycloud']:
    environment = 'ycloud'
elif env_type in ['notificator']:
    environment = 'notificator'
elif env_type in ['local']:
    environment = 'local'
else:
    raise Exception(f'Unknown environment type ({env_type})')


if "UNIPROXY_CUSTOM_SETTINGS_FILE" in os.environ:
    with open(os.environ.get("UNIPROXY_CUSTOM_SETTINGS_FILE")) as fin:
        config = json.load(fin)
else:
    config = load_settings(environment)


debug_logging = os.environ.get("UNIPROXY_DEBUG_LOGGING")
if debug_logging is not None:
    config["debug_logging"] = strtobool(debug_logging)


config["path"] = os.getcwd()
config["is_qloud"] = bool(IS_QLOUD)


config["disable_session_log"] = DISABLE_SESSION_LOG is not None


if not isinstance(config["vins"]["hosts_mapping"], collections.abc.Mapping):
    for rule in config["vins"]["hosts_mapping"]:
        rule[0] = re.compile(rule[0])

if 'notificator' not in env_type and 'delivery' not in env_type:
    config['ttsserver'] = load_tts_settings(environment)

    for rule in config["ttsserver"]["langs"]:
        rule[0] = re.compile(rule[0])

    config["ttsserver"]["host"] = os.environ.get('UNIPROXY_TTS_BACKEND', config["ttsserver"].get("host"))

config["yabio"]["host"] = os.environ.get('UNIPROXY_YABIO_BACKEND', config["yabio"].get("host"))
config["asr"]["yaldi_host"] = os.environ.get('UNIPROXY_YALDI_BACKEND', config["asr"].get("yaldi_host"))
config["vins"]["url"] = os.environ.get("UNIPROXY_VINS_BACKEND")
config["sentry"]["url"] = os.environ.get('UNIPROXY_SENTRY_URL', config["sentry"]["url"])
try:
    config["memcached"]["tts"]["enabled"] = strtobool(os.environ["UNIPROXY_MEMCACHED_TTS_ENABLED"])
except:
    pass


config['messenger']['anonymous_guid'] = os.environ.get(
    'UNIPROXY_MSSNGR_ANONYMOUS_GUID',
    config.get('messenger', {}).get('anonymous_guid', '')
)


if not try_load_experiments(config):
    logger.warning("Experiments configuration not found, will use configuration from %s" % (
                   config["experiments"].get("cfg_folder", "")
                   ))

qloud_tvm_token = os.environ.get('QLOUD_TVM_TOKEN')
tvm_app_secret = os.getenv('TVM_APP_SECRET')

tvm_config = json.loads(os.environ.get("QLOUD_TVM_CONFIG", "{}"))
tvm_client_id = tvm_config.get("clients", {}).get("uniproxy", {}).get("self_tvm_id", config["client_id"])

config["qloud_tvm_token"] = qloud_tvm_token
config["tvm_app_secret"] = tvm_app_secret
config["client_id"] = tvm_client_id

mssngr_ydb_pool = int(os.environ.get('MSSNGR_YDB_POOL', 0))
mssngr_ydb_rm_pool = int(os.environ.get('MSSNGR_YDB_RM_POOL', 0))

if mssngr_ydb_pool:
    config['messenger']['locator']['ydb']['pool_size'] = mssngr_ydb_pool
if mssngr_ydb_rm_pool:
    config['messenger']['locator']['ydb']['remove_pool_size'] = mssngr_ydb_rm_pool

config['messenger']['gzip_history'] = os.environ.get(
    'UNIPROXY_MESSENGER_GZIP_HISTORY',
    config['messenger'].get('gzip_history', False)
)

config["asr"]["lang_maps"] = load_json_file('lang_maps.json')

config["asr"]["topic_maps"] = load_json_file('topic_maps.json')

config["spotter_maps"] = load_json_file('spotter_topics.json')


def split_lang(lang):
    tk = lang.split('-', 1)
    if len(tk) == 1:
        return tk[0], tk[0].upper()
    return tk[0], tk[1]


class TopicMaps:
    """ inherit voiceproxy hack/logic for replace client selected topic
    """
    def __init__(self, topic_maps):
        self.maps = {}
        self.errors = []
        for it in topic_maps:
            topic = it.get('from')
            lang = it.get('lang')
            to = it.get('to')
            _list = it.get('list')
            if _list is not None:
                for it2 in _list:
                    topic2 = it2.get('from', topic)
                    lang2 = it2.get('lang', lang)
                    to2 = it2.get('to', to)
                    if topic2 is None or lang2 is None or to2 is None:
                        self.errors.append('invalid topic mapping topic_from={} topic_to={} lang={}\n'.format(
                            topic2, to2, lang2))
                        logger.error(self.errors[-1])
                        continue
                    l1, l2 = split_lang(lang2)
                    self.maps[(topic2, l1, l2)] = to2
            else:
                if topic is None or lang is None or to is None:
                    self.errors.append('invalid topic mapping topic_from={} topic_to={} lang={}\n'.format(
                        topic2, to2, lang2))
                    logger.error(self.errors[-1])
                    continue
                l1, l2 = split_lang(lang)
                self.maps[(topic, l1, l2)] = to

    def get(self, topic, lang):
        l1, l2 = split_lang(lang)
        mapped_topic = self.maps.get((topic, l1, l2))
        if mapped_topic:
            return mapped_topic
        mapped_topic = self.maps.get((topic, l1, '*'))
        if mapped_topic:
            return mapped_topic
        mapped_topic = self.maps.get((topic, '*', '*'))
        if mapped_topic:
            return mapped_topic
        return topic


class LangMaps:
    """ inherit voiceproxy hack/logic for replace client selected lang
    """
    def __init__(self, lang_maps):
        self.maps = {}
        self.errors = []
        for it in lang_maps:
            from_lang = it.get('from')
            to_lang = it.get('to')
            if from_lang is None or to_lang is None:
                self.errors.append('invalid lang mapping from={} to={}'.format(from_lang, to_lang))
                logger.error(self.errors[-1])
                continue
            l1, l2 = split_lang(it['from'])
            self.maps[(l1, l2)] = it['to']

    def get(self, lang):
        l1, l2 = split_lang(lang)
        mapped_lang = self.maps.get((l1, l2))
        if mapped_lang:
            return mapped_lang
        mapped_lang = self.maps.get((l1, '*'))
        if mapped_lang:
            return mapped_lang
        mapped_lang = self.maps.get(('*', '*'))
        if mapped_lang:
            return mapped_lang
        return lang


def make_immutable(d: dict):
    res = {}
    for k, v in d.items():
        if isinstance(v, (list, tuple)):
            res[k] = tuple(v)
        elif isinstance(v, collections.abc.Mapping):
            res[k] = make_immutable(v)
        else:
            res[k] = v
    return frozenishdict(res)


config = make_immutable(config)


TOPIC_MAPS = TopicMaps(config['asr']['topic_maps'])
LANG_MAPS = LangMaps(config['asr']['lang_maps'])
SPOTTER_MAPS = SpotterMaps(config['spotter_maps'])

QLOUD_HTTP_PORT = os.environ.get('QLOUD_HTTP_PORT', config['port'])

DELIVERY_DEFAULT_FORMAT = os.environ.get('DELIVERY_DEFAULT_FORMAT')
if not DELIVERY_DEFAULT_FORMAT:
    DELIVERY_DEFAULT_FORMAT = config['delivery']['format']

UNIPROXY_MEMVIEW_HANDLER_ENABLED = config.get('memview_handler', False)
