import argparse
import logging

from .context import Context
from .actions import *          # noqa
from .actions import Action, ActionBase, action


@action('proxycli.app.init_args')
class ApplicationInitArguments(ActionBase):
    async def execute(self, context: Context):
        parser = argparse.ArgumentParser()

        parser.add_argument('-c', '--config', metavar='FILE',
                            help='config file path')

        parser.add_argument('-s', '--scenario', metavar='NAME', required=True,
                            help='scenario name to run')

        parser.add_argument('-d', '--directory', metavar='PATH',
                            help='directory to search for scenarios instead of using embedded')

        parser.add_argument('-u', '--url', metavar='WS-URL', default='wss://uniproxy.tst.voicetech.yandex.net/uni.ws',
                            help='uniproxy websocket url')

        parser.add_argument('-t', '--token', metavar='KEY', default='developers-simple-key',
                            help='uniproxy client key')

        parser.add_argument('-a', '--oauth-token', metavar='TOKEN',
                            help='messenger/music oauth token')

        parser.add_argument('-r', '--reporting', action='store_true',
                            help='print report when done')

        parser.add_argument('-v', '--verbose', action='store_true',
                            help='enable debug logging')

        parser.add_argument('--debug-directives', action='store_true',
                            help='print all directives received from server')

        args = parser.parse_args()

        ctx = context['root']
        for key, value in args._get_kwargs():
            ctx.__setattr__(key, value)

        return True


@action('proxycli.app.init_logging')
class ApplicationInitLogging(ActionBase):
    async def execute(self, context: Context):
        logging.basicConfig(
            format='%(asctime)s %(levelname)8s %(name)-32s %(message)s',
            level=logging.DEBUG if context.verbose else logging.INFO
        )
        return True


@action('proxycli.app.init_logging_v2')
class ApplicationInitLoggingV2(ActionBase):
    async def execute(self, context: Context):
        logging.basicConfig(
            format='%(asctime)s %(name)-40s %(message)s',
            level=logging.DEBUG if context.verbose else logging.INFO
        )
        return True


@action('proxycli.app.run')
class ApplicationRun(ActionBase):
    async def execute(self, context: Context):
        scenario = context.scenarios.get(context.scenario)
        if scenario is None:
            context.log.error('failed to find scenario `%s`', context.scenario)
            return False

        ctx = context('app', tag='app', data=scenario)

        action = Action(ctx.action, ctx)
        if action:
            return await action()
        return False
