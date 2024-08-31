import collections
from dataclasses import dataclass

import yaml
import sys

from yatest import common as yc


DCS = {'man', 'sas', 'vla'}


@dataclass
class Alert:
    service: str
    alertset: str
    name: str
    config: dict

    def full_name(self):
        return f'{self.service}.{self.alertset}.{self.name}'

    def dc_from_name(self):
        suffix = self.name.split('_')[-1]
        return suffix if suffix in DCS else None

    def full_name_without_dc(self):
        dc = self.dc_from_name()
        if dc is not None:
            return self.full_name().rsplit('_', 1)[0]
        else:
            return self.full_name()


def iterate_alerts(skip_services=None):
    with open(yc.build_path('alice/paskills/alerts/monitorado.yml')) as f:
        monitorado_config = yaml.load(f.read())
    assert isinstance(monitorado_config, dict), 'Monitorado config should be a dictionary'
    for service, service_config in monitorado_config.items():
        if skip_services is not None and service in skip_services:
            continue
        if isinstance(service_config, dict) and 'alertsets' in service_config:
            for alertset_name, alertset in service_config['alertsets'].items():
                sys.stderr.write(f'{service}.{alertset_name}\n')
                for alert_key, alert in alertset['alerts'].items():
                    yield Alert(service, alertset_name, alert_key, alert)


def test_locations():
    for alert in iterate_alerts(skip_services=['nanny']):
        sys.stderr.write(alert.full_name())
        alert_name_dc = alert.dc_from_name()
        geo_tag = alert.config.get('tags', {}).get('geo')
        assert alert_name_dc == geo_tag, f'Alert {alert.full_name()} has invalid geo tag {geo_tag} (expected {alert_name_dc})'


def test_each_geo_alert_has_three_locations():
    alert_locations = collections.defaultdict(lambda: set())
    for alert in iterate_alerts():
        if alert.dc_from_name() is not None:
            alert_locations[alert.full_name_without_dc()].add(alert.dc_from_name())
    for alert_name, locations in alert_locations.items():
        assert locations == DCS, f'Alert {alert_name} is missing following locations: {DCS - locations}'
