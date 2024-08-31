from alice.paskills.alerts.solomon_alerts.registry.project.projects import paskills_alerts, dialogovo, memento
from library.python.monitoring.solo.objects.solomon.v2 import Channel, Method, Juggler

# через поля host и service связываются между собой алерты в Solomon и чеки в Juggler
paskills_juggler = Channel(
    id="juggler",
    project_id=paskills_alerts.id,
    name="Paskills Juggler",
    method=Method(
        juggler=Juggler(
            host="{{{annotations.host}}}",
            service="{{{annotations.service}}}",
            description=""
        )),
    notify_about_statuses={"NO_DATA",
                           "OK",
                           "ERROR",
                           "ALARM",
                           "WARN"},
)

dialogovo_juggler = Channel(
    id="dialogovo_juggler",
    project_id=dialogovo.id,
    name="Dialogovo Juggler",
    method=Method(
        juggler=Juggler(
            host="{{{annotations.host}}}",
            service="{{{annotations.service}}}",
            description=""
        )),
    notify_about_statuses={"NO_DATA",
                           "OK",
                           "ERROR",
                           "ALARM",
                           "WARN"},
)

memento_juggler = Channel(
    id="memento_juggler",
    project_id=memento.id,
    name="Memento Juggler",
    method=Method(
        juggler=Juggler(
            host="{{{annotations.host}}}",
            service="{{{annotations.service}}}",
            description=""
        )),
    notify_about_statuses={"NO_DATA",
                           "OK",
                           "ERROR",
                           "ALARM",
                           "WARN"},
)

# чтобы быть добавленными в общий registry, все объекты, которые мы хотим создать/модифицировать должны быть указаны в списке exports
exports = [
    paskills_juggler,
    dialogovo_juggler,
    memento_juggler
]
