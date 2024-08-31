# -*- coding: utf8 -*-
# flake8: noqa
"""
Graphic and Dashboard generator for Solomon
"""

import os

from alice.iot.dashboards.lib.solomon.time_machine import TimeMachineDashboards, TimeMachineGraphics
from alice.iot.dashboards.lib.solomon.bulbasaur import BulbasaurGraphics, BulbasaurDashboards, BulbasaurAlerts
from alice.iot.dashboards.lib.solomon.unified_agent import BulbasaurUnifiedAgentDashboards
from alice.iot.dashboards.lib.solomon.uxie import UxieGraphics, UxieDashboards, UxieAlerts
from alice.iot.dashboards.lib.solomon.provider import ProviderGraphics, ProviderDashboard
from alice.iot.dashboards.lib.solomon.tuya import TuyaGraphics, TuyaDashboards
from alice.iot.dashboards.lib.solomon.xiaomi import XiaomiGraphics, XiaomiDashboards
from alice.iot.dashboards.lib.solomon.vulpix import VulpixGraphics, VulpixDashboards


def get_oauth_token():
    token = os.getenv("SOLOMON_OAUTH_TOKEN", None)
    if not token:
        raise Exception("Environment variable SOLOMON_OAUTH_TOKEN is not set.")
    return token


SOLOMON_PROJECT = "alice-iot"

if __name__ == "__main__":
    OAUTH_TOKEN = get_oauth_token()
    # provider_graphics = ProviderGraphics(oauth_token=OAUTH_TOKEN)
    # provider_graphics.update_graphics(SOLOMON_PROJECT)
    #
    # provider_dashboards = ProviderDashboard(oauth_token=OAUTH_TOKEN)
    # provider_dashboards.update_dashboards(SOLOMON_PROJECT)

    # bulbasaur_graphics = BulbasaurGraphics(oauth_token=OAUTH_TOKEN)
    # bulbasaur_graphics.update_graphics(SOLOMON_PROJECT)

    # bulbasaur_dashboards = BulbasaurDashboards(oauth_token=OAUTH_TOKEN)
    # bulbasaur_dashboards.update_dashboards(SOLOMON_PROJECT)

    # unified_agent_graphs = BulbasaurUnifiedAgentGraphs(oauth_token=OAUTH_TOKEN)
    # unified_agent_graphs.update_graphics()
    # unified_agent_dashboards = BulbasaurUnifiedAgentDashboards(oauth_token=OAUTH_TOKEN)
    # unified_agent_dashboards.update_dashboards()

    # bulbasaur_alerts = BulbasaurAlerts(oauth_token=OAUTH_TOKEN)
    # bulbasaur_alerts.update_alerts(SOLOMON_PROJECT)

    # uxie_graphics = UxieGraphics(oauth_token=OAUTH_TOKEN)
    # uxie_graphics.update_graphics(SOLOMON_PROJECT)

    # uxie_dashboards = UxieDashboards(oauth_token=OAUTH_TOKEN)
    # uxie_dashboards.update_dashboards(SOLOMON_PROJECT)

    # uxie_alerts = UxieAlerts(oauth_token=OAUTH_TOKEN)
    # uxie_alerts.update_alerts(SOLOMON_PROJECT)

    # tuya_graphics = TuyaGraphics(oauth_token=OAUTH_TOKEN)
    # tuya_graphics.update_graphics(SOLOMON_PROJECT)

    # tuya_dashboards = TuyaDashboards(oauth_token=OAUTH_TOKEN)
    # tuya_dashboards.update_dashboards(SOLOMON_PROJECT)
    #
    # xiaomi_graphics = XiaomiGraphics(oauth_token=OAUTH_TOKEN)
    # xiaomi_graphics.update_graphics(SOLOMON_PROJECT)
    #
    # xiaomi_dashboards = XiaomiDashboards(oauth_token=OAUTH_TOKEN)
    # xiaomi_dashboards.update_dashboards(SOLOMON_PROJECT)

    # timemachine_graphics = TimeMachineGraphics(oauth_token=OAUTH_TOKEN)
    # timemachine_graphics.update_graphics(SOLOMON_PROJECT)

    # timemachine_dashboards = TimeMachineDashboards(oauth_token=OAUTH_TOKEN)
    # timemachine_dashboards.update_dashboards(SOLOMON_PROJECT)

    # vulpix_graphics = VulpixGraphics(oauth_token=OAUTH_TOKEN)
    # vulpix_graphics.update_graphics(SOLOMON_PROJECT)

    # vulpix_dashboards = VulpixDashboards(oauth_token=OAUTH_TOKEN)
    # vulpix_dashboards.update_dashboards(SOLOMON_PROJECT)
