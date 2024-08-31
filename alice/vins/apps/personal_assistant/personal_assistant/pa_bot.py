# coding: utf-8
from __future__ import unicode_literals

import logging
import logging.config
import argparse

from vins_core.utils.config import get_setting, get_bool_setting
from vins_core.utils.data import find_vinsfile
from vins_core.dm.session import InMemorySessionStorage
from vins_core.utils.updater import start_all_updaters

from vins_sdk.connectors import TelegramConnector

from personal_assistant.app import PersonalAssistantApp

logger = logging.getLogger(__name__)
logging.getLogger('telegram.bot').setLevel(logging.ERROR)


def main(in_telegram_token):
    session_storage = InMemorySessionStorage()
    app = PersonalAssistantApp(
        vins_file=find_vinsfile('personal_assistant'),
        app_id='personal_assistant',
        session_storage=session_storage,
    )

    # run separate threads in each fork
    if get_bool_setting('ENABLE_BACKGROUND_UPDATES'):
        start_all_updaters()

    bot_connector = TelegramConnector(in_telegram_token, vins_app=app)
    bot_connector.run()


def run_bot():
    logging.config.dictConfig({
        'version': 1,
        'disable_existing_loggers': False,
        'formatters': {
            'standard': {
                'format': '[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s',
            },
            'qloud_json': {
                '()': 'vins_core.logger.QloudJsonFormatter',
            },
        },
        'handlers': {
            'console': {
                'class': 'logging.StreamHandler',
                'stream': 'ext://sys.stdout',
                'level': 'DEBUG',
                'formatter': (
                    'qloud_json'
                    if get_setting('QLOUD_LOGGER_STDOUT_PARSER', '', prefix='') == 'json'
                    else 'standard'
                ),
            },
        },
        'loggers': {
            '': {
                'handlers': ['console'],
                'level': 'DEBUG',
                'propagate': True,
            },
        },
    })
    parser = argparse.ArgumentParser()
    parser.add_argument('--telegram_token', required=True)
    args = parser.parse_args()
    main(args.telegram_token)


if __name__ == "__main__":
    run_bot()
