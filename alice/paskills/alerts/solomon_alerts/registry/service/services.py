from alice.paskills.alerts.solomon_alerts.registry.project import projects
from library.python.monitoring.solo.objects.solomon.v2 import Service

billing = Service(
    id='paskills_billing_billing',
    project_id=projects.billing.id,
    name='billing',
)

dialogovo_main_pulling = Service(
    id='dialogovo_main_pulling',
    project_id=projects.dialogovo.id,
    name='main_pulling',
)

memento_main_monitoring = Service(
    id='memento_main_monitoring',
    project_id=projects.memento.id,
    name='main_monitoring',
)

my_alice_backend = Service(
    id='my_alice_backend',
    project_id=projects.my_alice.id,
    name='backend',
)

my_alice_pumpkin = Service(
    id='my_alice_pumpkin',
    project_id=projects.my_alice.id,
    name='pumpkin',
)

services = [s for s in locals().values() if isinstance(s, Service)]

service_by_id = {s.id: s for s in services}

# чтобы быть добавленными в общий regisry, все объекты, которые мы хотим создать/модифицировать должны быть указаны в списке exports
exports = [
]
