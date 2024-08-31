from itertools import chain

from alice.paskills.alerts.solomon_alerts.registry.dashboard.dashboards import exports


def get_all_dashboards():
    return list(
        chain(
            exports
        )
    )
