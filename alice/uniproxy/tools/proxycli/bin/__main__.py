import sys

from alice.uniproxy.tools.proxycli.library.proxycli.actions import *                # noqa
from alice.uniproxy.tools.proxycli.library.proxycli.actions_scenario import *       # noqa
from alice.uniproxy.tools.proxycli.library.proxycli.actions_application import *    # noqa
from alice.uniproxy.tools.proxycli.library.proxycli.actions_proxycli import *       # noqa
from alice.uniproxy.tools.proxycli.library.proxycli.actions_proxy import *          # noqa
from alice.uniproxy.tools.proxycli.library.proxycli.actions_report import *         # noqa

from alice.uniproxy.tools.proxycli.library.proxycli.utils import run_scenario_from_resource


if __name__ == '__main__':
    sys.exit(0 if run_scenario_from_resource('/scenarios/application.yaml') else 1)
