# coding: utf-8
from __future__ import unicode_literals

import argparse
import logging
import logging.config

from vins_sdk.app import VinsApp
from vins_sdk.connectors import TelegramConnector
from vins_core.utils.config import get_setting
from vins_core.dm.response import VinsResponse
from vins_core.dm.session import DummySessionStorage
from vins_core.common.utterance import Utterance

from navi_app.lib.mockdata import generate_options, choose_location
from navi_app.lib.adhoc import adhoc_nlu_handle
from navi_app.lib.callbacks import callbacks_map, simple_directive
from navi_app.lib.navirequest import normalize_lang
from navi_app.lib.tvm_client import tvm_client_factory

logger = logging.getLogger(__name__)

TVM2_NAME = get_setting('TVM2_NAME', default='navi_tvm')
TVM2_CLIENT_ID = get_setting('TVM2_CLIENT_ID', default=2001948)
TVM2_SECRET = get_setting('TVM2_SECRET', default='')
TVM2_GEO_CLIENT_ID = get_setting('TVM2_GEO_CLIENT_ID', yenv={
    'production': 2001886,
    'testing': 2008261,
    'development': 2008261,
})

tvm_client_factory.add_tvm_client(
    name=TVM2_NAME, client_id=int(TVM2_CLIENT_ID), secret=TVM2_SECRET, destinations={'geo': int(TVM2_GEO_CLIENT_ID)}
)


class NaviApp(VinsApp):
    def __init__(self, vins_file=None, session_storage=None, app_id='navi', **kwargs):
        vins_file = vins_file or 'navi_app/config/Vinsfile.json'
        session_storage = session_storage or DummySessionStorage()
        super(NaviApp, self).__init__(
            vins_file=vins_file,
            session_storage=session_storage,
            app_id=app_id,
            **kwargs
        )
        for c in callbacks_map:
            self.register_callback(c, callbacks_map[c])

    def handle_request(self, req_info, **kwargs):
        # Set default data for telegram requests
        if (
            req_info.utterance.text in ['', ' '] or
            req_info.additional_options.get('broken', False) or
            req_info.utterance.input_source not in [Utterance.VOICE_INPUT_SOURCE, Utterance.TEXT_INPUT_SOURCE, Utterance.SUGGESTED_INPUT_SOURCE]
        ):
            # Handle empty requests and broken context
            response = self._dont_understand_response(req_info, **kwargs)
        elif normalize_lang(req_info.lang) in ['tr', 'en', 'fr', 'he']:
            # Handle foreign langs [tr, en, fr]. TODO: support multilanguage nlu
            client_action = adhoc_nlu_handle(req_info, **kwargs)
            response = VinsResponse()
            response.directives.append(client_action)
            response.say(client_action.payload['text'])
        else:
            # Handle ru, uk. Catch all exceptions from vins_core to avoid 500 responses to client.
            try:
                response = super(NaviApp, self).handle_request(req_info, **kwargs)
            except Exception:
                response = self._dont_understand_response(req_info, **kwargs)
                logger.error('Handler failed', exc_info=True)

        return response

    def _dont_understand_response(self, req_info, **kwargs):
        response = VinsResponse()
        action = simple_directive(req_info, 'dont_understand')
        response.directives.append(action)
        response.say(action.payload['text'])
        return response


def main(telegram_token):
    lang = get_setting('DEFAULT_LANG', 'ru')
    location = choose_location(lang)
    bot_connector = TelegramConnector(
        telegram_token,
        vins_app=NaviApp(),
        req_info_args={
            'location': location,
            'additional_options': generate_options(lang, location),  # Fill span, favourites
        }
    )
    bot_connector.run()


def run_bot(log_level='INFO'):
    logging.config.dictConfig({
        'version': 1,
        'disable_existing_loggers': False,
        'formatters': {
            'standard': {
                'format': '[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s',
            },
        },
        'handlers': {
            'console': {
                'class': 'logging.StreamHandler',
                'level': log_level,
                'formatter': 'standard',
            },
        },
        'loggers': {
            '': {
                'handlers': ['console'],
                'level': 'DEBUG',
                'propagate': True,
            },
            'telegram.bot': {
                'handlers': ['console'],
                'level': 'CRITICAL',
                'propagate': True,
            },
            'vins_sdk.connectors': {
                'handlers': ['console'],
                'level': 'CRITICAL',
                'propagate': True,
            }
        },
    })
    parser = argparse.ArgumentParser()
    parser.add_argument('--telegram_token', required=True)
    args = parser.parse_args()

    main(args.telegram_token)


def base_handler_fail_mock(obj, req_info, **kwargs):
    raise RuntimeError()


if __name__ == '__main__':
    run_bot(log_level='DEBUG')
