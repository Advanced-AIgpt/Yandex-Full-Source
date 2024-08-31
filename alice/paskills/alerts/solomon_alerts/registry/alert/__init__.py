from itertools import chain

from alice.paskills.alerts.solomon_alerts.registry.alert.paskills_alerts import exports as paskills_exports
from alice.paskills.alerts.solomon_alerts.registry.alert.dialogovo_alerts import exports as dialogovo_exports
from alice.paskills.alerts.solomon_alerts.registry.alert.dialogovo_alerts_from_megamind import exports as megamind_exports
from alice.paskills.alerts.solomon_alerts.registry.alert.memento_alerts import exports as memento_exports


def get_all_alerts():
    return list(chain(
        paskills_exports,
        dialogovo_exports,
        megamind_exports,
        memento_exports,
    ))
