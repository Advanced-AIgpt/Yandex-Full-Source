from typing import List

from library.python.monitoring.solo.objects.solomon.v2 import Cluster
from alice.paskills.alerts.solomon_alerts.registry.cluster import clusters
from alice.paskills.alerts.solomon_alerts.registry.project import projects
from alice.paskills.alerts.solomon_alerts.registry.service import services
from library.python.monitoring.solo.objects.solomon.v2 import Shard


dialogovo_prod_main_pulling = Shard(
    id="dialogovo_prod_main_pulling",
    project_id=projects.dialogovo.id,
    cluster_id=clusters.dialogovo_prod.id,
    service_id=services.dialogovo_main_pulling.id,
)

billing_prod = Shard(
    id='paskills_billing_prod_billing',
    project_id=projects.billing.id,
    cluster_id=clusters.billing_prod.id,
    service_id=services.billing.id,
)

memento_prod = Shard(
    id='memento_production_main_monitoring',
    project_id=projects.memento.id,
    cluster_id=clusters.memento_prod.id,
    service_id=services.memento_main_monitoring.id,
)

my_alice_prod = Shard(
    id='my_alice_production_backend',
    project_id=projects.my_alice.id,
    cluster_id=clusters.my_alice_prod.id,
    service_id=services.my_alice_backend.id,
)

my_alice_pumpkin_prod = Shard(
    id='my_alice_pumpkin_prod_pumpkin',
    project_id=projects.my_alice.id,
    cluster_id=clusters.my_alice_pumpkin_prod.id,
    service_id=services.my_alice_pumpkin.id,
)

jvm_metric_shards: List[Cluster] = [
    dialogovo_prod_main_pulling,
    billing_prod,
    memento_prod,
    my_alice_prod,
    my_alice_pumpkin_prod,
]


# чтобы быть добавленными в общий regisry, все объекты, которые мы хотим создать/модифицировать должны быть указаны в списке exports
exports = [
]
