import argparse
from collections import OrderedDict
import json
from library.python import resource
import logging
import logging.config
import os

import werkzeug
from flask import Flask, request, jsonify, abort, render_template

from verstehen.app.search_app import UtteranceSearchApp
from verstehen.config import GenericConfig, validate_server_config
from verstehen.granet_errors import WrongGrammar, handle_wrong_grammar
from verstehen.markup import process_markup_request, vh_setup_main_thread

logger = logging.getLogger(__name__)

flask_app = Flask(__name__)

flask_app.register_error_handler(WrongGrammar, handle_wrong_grammar)


@flask_app.errorhandler(werkzeug.exceptions.InternalServerError)
def handle_bad_request(e):
    return str(e), 500


@flask_app.route('/static/<file_name>', methods=['GET'])
def get_static_file(file_name):
    found_resource = resource.find('resource/static/{}'.format(file_name))
    if found_resource is not None:
        return found_resource
    else:
        abort(404)


@flask_app.route('/', methods=['GET'])
def search_page():
    return get_static_file('web_interface.html')


@flask_app.route('/granet', methods=['GET'])
def search_granet_page():
    return render_template('index.html')


@flask_app.route('/apps', methods=['GET'])
def get_current_apps():
    global current_apps_mapping
    return jsonify(current_apps_mapping)


@flask_app.route('/search', methods=['POST'])
def search():
    global server_config
    payload = request.get_json()
    return process_search_request(payload['app_name'],
                                  payload['index_name'],
                                  payload['query'],
                                  payload.get('n_samples', server_config['default_n_samples']),
                                  payload.get('filters', []))


@flask_app.route('/search_by_skill_id', methods=['POST'])
def search_by_skill_id():
    global server_config
    payload = request.get_json()
    return process_search_request(payload['app_name'],
                                  payload['index_name'],
                                  payload['query'],
                                  payload.get('n_samples', server_config['default_n_samples']),
                                  payload.get('filters', []),
                                  skill_id=payload['skill_id'])


def process_search_request(app_name, index_name, query, n_samples, filters, skill_id=None):
    global search_app_map
    search_app = search_app_map[app_name]
    search_results = search_app.search(
        query,
        n_samples=n_samples,
        index_name=index_name,
        filters=filters,
        skill_id=skill_id
    )
    response = {
        'search_result': search_results,
        'index_name': index_name,
        'app_name': app_name
    }
    return jsonify(response)


@flask_app.route('/estimate', methods=['POST'])
def estimate():
    global server_config
    payload = request.get_json()
    return process_estimate_request(payload['app_name'],
                                    payload['index_name'],
                                    payload['query'],
                                    payload.get('n_samples', server_config['default_n_samples']),
                                    payload.get('filters', []))


@flask_app.route('/estimate_by_skill_id', methods=['POST'])
def estimate_by_skill_id():
    global server_config
    payload = request.get_json()
    return process_estimate_request(payload['app_name'],
                                    payload['index_name'],
                                    payload['query'],
                                    payload.get('n_samples', server_config['default_n_samples']),
                                    payload.get('filters', []),
                                    skill_id=payload['skill_id'])


def process_estimate_request(app_name, index_name, query, n_samples, filters, skill_id=None):
    search_app = search_app_map[app_name]

    results = search_app.estimate(
        query, index_name=index_name, filters=filters, skill_id=skill_id)
    logger.debug('Result size {}'.format(len(results)))
    response = {
        'search_result': results,
        'index_name': index_name,
        'app_name': app_name
    }
    return jsonify(response)


@flask_app.route('/markup', methods=['POST'])
def markup():
    # Need this try and except closure as an error may lead to an automatic retry however an error
    # does not necessarily show that the markup was not launched (nirvana is a strange animal)
    try:
        nirvana_url = process_markup_request(request.get_json())
        return jsonify({'nirvana_url': nirvana_url})
    except Exception as ex:
        logger.error(ex)
        error_text = 'The markup request has failed, so we cannot retrieve Nirvana URL, but probably the graph has ' \
                     'already been created and launched. Before retrying the request please check your Nirvana page.'
        return error_text, 449  # retry code


def init(config):
    global search_app_map, server_config, current_apps_mapping, flask_app
    search_app_map = OrderedDict()
    server_config = config['server']

    logging.config.dictConfig({
        'version': 1,
        'disable_existing_loggers': False,
        'formatters': {
            'default': {
                'format': '%(asctime)s %(levelname)s:%(name)s %(message)s',
                'datefmt': '%Y-%m-%d %H:%M:%S',
                'class': 'logging.Formatter',
            },
            'json': {
                '()': 'ylog.format.QloudJsonFormatter',
            },
        },
        'handlers': {
            'default': {
                'class': 'logging.StreamHandler',
                'formatter': 'default',
            },
            'qloud': {
                'class': 'logging.StreamHandler',
                'formatter': 'json',
            },
        },
        'loggers': {
            '': {
                'level': logging.DEBUG if server_config['verbose'] else logging.WARN,
                'handlers': ['qloud'] if server_config.get('qloud_logging', False) else ['default'],
                'propagate': True,
            },
        },
    })

    # creating all necessary apps and indexes
    for app_config in config['apps']:
        app_name = app_config['name']
        app = UtteranceSearchApp.from_config(app_config)
        search_app_map[app_name] = app

    # configuring apps mapping to show what apps and indexes are loaded into the server (current_apps_mapping)
    app_names_with_indexes = dict()
    for app_config in config['apps']:
        indexes_configs = app_config['indexes']
        indexes = [conf['name'] for conf in indexes_configs]
        default_index = app_config.get(
            'default_index', indexes_configs[0]['name'])

        if default_index not in indexes:
            raise ValueError('Default index {} must be among existing indexes: {}'.format(
                default_index, indexes))

        app_names_with_indexes[app_config['name']] = {
            'indexes': indexes,
            'default_index': default_index
        }

    # either specified default_app or first app
    default_app = config.get('default_app', config['apps'][0]['name'])
    if default_app not in app_names_with_indexes:
        raise ValueError(
            'Default app {} must be among existing apps: {}'.format(default_app, app_names_with_indexes.keys()))

    current_apps_mapping = {
        'all_apps': app_names_with_indexes,
        'default_app': default_app
    }

    # vh needs to be set up in main thread in order to be used in other threads during requests
    vh_setup_main_thread()

    if 'granet_ui_dir' in server_config:
        flask_app.template_folder = os.path.abspath(
            os.path.join(server_config['granet_ui_dir'], 'build'))
        flask_app.static_folder = os.path.abspath(
            os.path.join(server_config['granet_ui_dir'], 'build', 'static'))

    # IPv6 host name
    flask_app.run(host='::', port=server_config['port'])


def read_config(path):
    with open(path) as f:
        config = json.load(f)
    return config


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--config_path', type=str, default=None,
        help='Path to JSON config for the app.'
    )
    parser.add_argument(
        '--port', type=int, default=None,
        help='Port to run the server on. Overrides port from config if present.'
    )
    args = parser.parse_args()

    if args.config_path is not None:
        logger.debug('Reading config from {} file'.format(args.config_path))
        config = read_config(args.config_path)
    else:
        logger.debug('Config path is not specified, falling to generic config from {}'.format(
            GenericConfig.__name__))
        config = GenericConfig.DEFAULT_SERVER_CONFIG

    # overriding port value if specified
    if args.port is not None:
        config['server']['port'] = args.port

    config = validate_server_config(config)
    init(config)
