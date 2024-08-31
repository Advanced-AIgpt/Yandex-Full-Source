import asyncio
import logging
import os
import time
import json


from .context import Context


__log = logging.getLogger('proxycli.action')
__action_registry = {}

__PROXYCLI_LOG_REGISTRY = os.environ.get('PROXYCLI_LOG_REGISTRY')


def action(name):
    """ register an action """
    def action_wrapper(cls):
        global __log, __action_registry
        if __PROXYCLI_LOG_REGISTRY:
            print('registering action: %s', name)
        __action_registry[name] = cls
        return cls
    return action_wrapper


def Action(name, context: Context):
    """ create a registered action by name """
    global __action_registry
    if name in __action_registry:
        if __PROXYCLI_LOG_REGISTRY:
            print('creating an action: %s', name)
        return __action_registry[name](context)
    if __PROXYCLI_LOG_REGISTRY:
        print('creating an action %s failed', name)
    return None


class ActionBase(object):
    def __init__(self, context: Context):
        super().__init__()
        self._context = context
        self._context.report = self._context('report', tag='report')
        self._context.report.action = self.__class__.__name__

        description = context.get_current('description')
        if description is not None:
            self._context.report.description = description
        self._context.log = logging.getLogger(str(self._context))

    def __pre__(self, c: Context):
        c.report.started_at = time.time()
        c.report.finished_at = c.report.started_at

    def __post__(self, c: Context):
        c.report.finished_at = time.time()
        c.report.duration = c.report.finished_at - c.report.started_at

    async def pre_execute(self, context: Context):
        pass

    async def post_execute(self, context: Context):
        pass

    async def execute(self, context: Context):
        pass

    async def __call__(self):
        ret = False
        self.__pre__(self._context)
        try:
            await self.pre_execute(self._context)
            ret = await self.execute(self._context)
            await self.post_execute(self._context)
        except Exception as ex:
            self._context.log.exception('__call__: %s', ex)
        self.__post__(self._context)
        return ret


@action('proxycli.sleep')
class SleepAction(ActionBase):
    async def execute(self, context: Context):
        sleep_time = context.get('value', 1.0)
        context.log.debug('sleeping for %.3f seconds...', sleep_time)
        await asyncio.sleep(sleep_time)
        context.log.debug('sleeping for %.3f seconds... DONE!', sleep_time)


@action('proxycli.debug')
class DebugAction(ActionBase):
    async def execute(self, context: Context):
        if context.message:
            context.log.info(context.message)


@action('proxycli.debug_context')
class DebugContext(ActionBase):
    async def execute(self, context: Context):
        print(json.dumps(context['root'].to_dict(), indent=2))
