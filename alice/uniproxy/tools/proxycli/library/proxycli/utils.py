import asyncio
import yaml

from .actions import *              # noqa
from .actions_scenario import *     # noqa
from .actions_application import *  # noqa

from .context import Context
from .actions import Action


from library.python import resource


def run_scenario_from_resource(name):
    scenario = resource.find(name)

    if scenario is None:
        return False

    scenario = yaml.load(scenario, Loader=yaml.FullLoader)
    context = Context('root', tag='root', data=scenario)

    action = Action(scenario.get('action'), context)

    if action is not None:
        return asyncio.get_event_loop().run_until_complete(action())
    else:
        return False
