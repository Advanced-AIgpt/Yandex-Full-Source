from library.python.monitoring.solo.objects.solomon.v2 import Project

default_kwargs = {
    'abc_service': 'yandexdialogs2',
    'only_auth_push': True,
    'only_sensor_name_shards': False,
    'only_new_format_writes': False,
    'only_new_format_reads': False,
}

# обычно проект не контролируется Соло (проект уже создан), но нужен как объект для связи остальных сущностей
billing = Project(
    id='paskills_billing',
    name='paskills_billing',
    **default_kwargs,
)

dialogovo = Project(
    id='dialogovo',
    name='dialogovo',
    **default_kwargs,
)

memento = Project(
    id='memento',
    name='memento',
    **default_kwargs,
)

paskills_alerts = Project(
    id="paskills-alerts",
    name="paskills-alerts",
    **default_kwargs,
)

my_alice = Project(
    id='my_alice',
    name='my_alice',
    **default_kwargs,
)

projects = [p for p in locals().values() if isinstance(p, Project)]

project_by_id = {p.id: p for p in projects}

java_projects = [
    dialogovo,
    billing,
    memento,
]

exports = [
    paskills_alerts,
]
