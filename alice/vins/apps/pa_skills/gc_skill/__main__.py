# coding: utf-8

from __future__ import unicode_literals

import logging
import argparse

from vins_core.utils.data import find_vinsfile
from vins_core.dm.session import InMemorySessionStorage

from vins_sdk.connectors import TelegramConnector
from gc_skill.app import ExternalSkillApp

logger = logging.getLogger(__name__)
logging.getLogger('telegram.bot').setLevel(logging.ERROR)


def main(in_telegram_token):
    session_storage = InMemorySessionStorage()
    app = ExternalSkillApp(
        vins_file=find_vinsfile('gc_skill'),
        app_id='gc_skill',
        session_storage=session_storage,
    )
    bot_connector = TelegramConnector(in_telegram_token, vins_app=app)
    bot_connector.run()


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)

    parser = argparse.ArgumentParser()
    parser.add_argument('--telegram_token', required=True)
    args = parser.parse_args()

    main(args.telegram_token)
