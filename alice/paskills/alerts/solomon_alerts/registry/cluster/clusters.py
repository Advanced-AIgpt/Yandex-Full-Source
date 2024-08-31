from alice.paskills.alerts.solomon_alerts.registry.project import projects
from library.python.monitoring.solo.objects.solomon.v2 import Cluster


dialogovo_prod = Cluster(
    id='dialogovo_prod',
    project_id=projects.dialogovo.id,
    name='prod',
)


billing_prod = Cluster(
    id='paskills_billing_prod',
    project_id=projects.billing.id,
    name='prod',
)


memento_prod = Cluster(
    id='memento_production',
    project_id=projects.memento.id,
    name='production',
)


my_alice_prod = Cluster(
    id='my_alice_production',
    project_id=projects.my_alice.id,
    name='production'
)


my_alice_pumpkin_prod = Cluster(
    id='my_alice_pumpkin_prod',
    project_id=projects.my_alice.id,
    name='pumpkin_prod'
)


clusters = [c for c in locals().values() if isinstance(c, Cluster)]


cluster_by_id = {c.id: c for c in clusters}


# чтобы быть добавленными в общий regisry, все объекты, которые мы хотим создать/модифицировать должны быть указаны в списке exports
exports = [
]
