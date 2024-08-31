import asyncio

from .actions import *          # noqa
from .context import Context

from .actions import Action, ActionBase, action

from library.python import resource

import os
import yaml


@action('proxycli.scenarios.load_from_directory')
class DirectoryScenarioLoader(ActionBase):
    async def execute(self, context: Context):
        scenarios = {}
        directory = os.path.relpath(context.directory)
        for name in os.listdir(directory):
            path = os.path.join(directory, name)
            if path.endswith('.yaml'):
                with open(path, 'r') as fin:
                    try:
                        scenario = yaml.load(fin, Loader=yaml.FullLoader)
                        scenario_name = os.path.basename(path)
                        scenario_name, _ = os.path.splitext(scenario_name)
                        scenarios[scenario_name] = scenario
                        context.log.debug('scenario `%s` is loaded', scenario_name)
                    except Exception as ex:
                        context.log.error('failed to load scenario %s: %s', path, str(ex))
                        continue

        context['root'].scenarios = scenarios


@action('proxycli.scenarios.load_from_resource')
class ResourceScenarioLoader(ActionBase):
    async def execute(self, context: Context):
        prefix = context.get('scenario_prefix', '/scenarios/')

        scenarios = {}
        for key, data in resource.iteritems():
            if key.startswith(prefix) and key.endswith('.yaml'):
                try:
                    scenario = yaml.load(data, Loader=yaml.FullLoader)
                    scenario_name = os.path.basename(key)
                    scenario_name, _ = os.path.splitext(scenario_name)
                    scenarios[scenario_name] = scenario
                    context.log.debug('scenario `%s` is loaded', scenario_name)
                except Exception as ex:
                    context.log.error('failed to load scenario %s: %s', key, str(ex))
                    continue

        context['root'].scenarios = scenarios


@action('proxycli.scenarios.load')
class ScenarioLoader(ActionBase):
    async def execute(self, context: Context):
        context['root'].scenarios = {}
        if context.directory:
            action = DirectoryScenarioLoader(context('loader'))
        else:
            action = ResourceScenarioLoader(context('loader'))

        await action()


@action('scenario')
class Scenario(ActionBase):
    async def execute(self, context: Context):
        for step in context.steps:
            ctx = context('stp', tag='step', data=step)
            act = Action(ctx.action, ctx)
            if act:
                await act()
            else:
                return False
        return True


@action('scenario.run')
class ScenarioRun(ActionBase):
    async def execute(self, context: Context):
        name = context.name
        scenario = context.scenarios.get(name)
        context.report.message = name
        if scenario:
            worker = Scenario(context('scenario', tag='scenario', data=scenario))
            await worker()
            context.report.ok = True
        else:
            context.report.ok = False


@action('scenario.parallel')
class ScenarioParallel(ActionBase):
    async def execute(self, context: Context):
        actions = []

        for step in context.steps:
            ctx = context('parallel', tag='step', data=step)
            act = Action(ctx.action, ctx)
            if act:
                actions.append(act())
            else:
                return False

        results = await asyncio.gather(*actions)

        return all(results)
