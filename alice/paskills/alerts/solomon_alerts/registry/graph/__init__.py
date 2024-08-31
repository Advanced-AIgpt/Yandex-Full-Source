from itertools import chain

from alice.paskills.alerts.solomon_alerts.registry.graph.graphs import exports


def get_all_graphs():
    """
    Provides set of local graphs instances
    :return:
    """
    return list(
        chain(
            exports
        )
    )
