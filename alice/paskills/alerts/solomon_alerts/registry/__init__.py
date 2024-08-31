import itertools

from alice.paskills.alerts.solomon_alerts.registry.alert import get_all_alerts
from alice.paskills.alerts.solomon_alerts.registry.channel import get_all_channels
from alice.paskills.alerts.solomon_alerts.registry.cluster import get_all_clusters
from alice.paskills.alerts.solomon_alerts.registry.dashboard import get_all_dashboards
from alice.paskills.alerts.solomon_alerts.registry.graph import get_all_graphs
from alice.paskills.alerts.solomon_alerts.registry.service import get_all_services
from alice.paskills.alerts.solomon_alerts.registry.shard import get_all_shards

# импортируем все объекты Соло и собираем в один общий список registry
# потом registry подается на вход контроллерам Solomon и Juggler
objects_registry = list(itertools.chain(
    get_all_clusters(),
    get_all_services(),
    get_all_shards(),
    get_all_alerts(),
    get_all_channels(),
    get_all_graphs(),
    get_all_dashboards(),
))
