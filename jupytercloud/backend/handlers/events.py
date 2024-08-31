import datetime

from tornado import web
from traitlets import Instance, List, Unicode
from traitlets.config import SingletonConfigurable

from jupytercloud.backend.lib.clients.infra import InfraClient, InfraFilter

# TODO: Make it configurable somehow, because QYPClient allow to configure it
from jupytercloud.backend.lib.clients.qyp import VMPROXY_URL_MAP
from jupytercloud.backend.lib.qyp.vm import QYPVirtualMachine
from jupytercloud.backend.lib.util.misc import cached_class_property

from .base import JCAPIHandler, JCPageHandler
from .utils import NAME_RE, require_server


JUPYTER_FILTER = InfraFilter(
    service_id=931,  # JupyterCloud
    environment_id=1271,  # Production
    duration=14,
    type_='maintenance',
    severity='major',
)

DC_OUTAGE_FILTER = InfraFilter(
    service_id=154,  # DC outage
    environment_id=204,  # Drills
    duration=14,
)


class EventsConfigurable(SingletonConfigurable):
    infra_default_filters = List(
        Instance(InfraFilter),
        [JUPYTER_FILTER, DC_OUTAGE_FILTER],
        config=True,
    )

    infra_filters = List(Instance(InfraFilter), config=True)

    min_duration = Instance(
        datetime.timedelta,
        kw=dict(hours=1),
    )

    username = Unicode()
    vm = Instance(QYPVirtualMachine)

    future_unavailable_message = Unicode(
        'Your VM is going to be unavailable'
        ' from {start_datetime} to {finish_datetime}'
        ' (will take {hours} hours)'
        ' due to a planned maintenance in locations: {locations}.',
        config=True,
    )

    unavailable_message = Unicode(
        'Your VM is unavailable'
        ' from {start_datetime} to {finish_datetime}'
        ' ({hours} hours left)'
        ' due to a planned maintenance in locations: {locations}.',
        config=True,
    )

    migration_message = Unicode(
        "If it's important to you,"
        ' you can migrate your VM to another location ({free_locations}).',
        config=True,
    )
    migration_doc_url = Unicode(config=True)

    datetime_format = Unicode(
        '%Y-%m-%d %H:%M',
        config=True,
    )

    @cached_class_property
    def infra_client(self):
        return InfraClient.instance(
            config=self.config,
        )

    @property
    def data_center(self):
        # jupyter-cloud-lipkin.sas.yp-c.yandex.net
        parts = self.vm.host.rsplit('.', 4)
        return parts[1].lower()

    def format_datetime(self, timestamp):
        # Our servers configured in Europe/Moscow timezone.
        # So I won't think about timezones here at the moment.
        dt = datetime.datetime.fromtimestamp(timestamp)
        return dt.strftime(self.datetime_format)

    async def get(self):
        filters = self.infra_default_filters + self.infra_filters

        raw_events = await self.infra_client.get_events(*filters)

        result = []

        for raw_event in raw_events:
            if not raw_event.get(self.data_center):
                continue

            # finish time is not set
            if not raw_event.get('finish_time'):
                continue

            finish_time = raw_event['finish_time']
            start_time = raw_event['start_time']
            planned_duration = finish_time - start_time
            if planned_duration < self.min_duration.total_seconds():
                continue

            now = datetime.datetime.now().timestamp()
            ongoing = raw_event['start_time'] < now
            duration = finish_time - max((now, start_time))

            hours = round(duration / 3600)

            dc_statuses = {dc: raw_event.get(dc) for dc in VMPROXY_URL_MAP}
            locations = [dc for dc, status in dc_statuses.items() if status]
            # todo: suggest a free personalised location here
            free_locations = [dc for dc, status in dc_statuses.items() if not status]

            event = {
                'event_url': self.infra_client.get_event_url(raw_event['id']),
                'original': raw_event,
                'host': self.vm.host,
                'start_datetime': self.format_datetime(raw_event['start_time']),
                'finish_datetime': self.format_datetime(raw_event['finish_time']),
                'hours': hours,
                'locations': ', '.join(locations),
                'free_locations': ', '.join(free_locations),
                'ongoing': ongoing,
            }

            lines = [
                self.unavailable_message
                if ongoing else
                self.future_unavailable_message,
            ]

            if free_locations and not ongoing:
                lines.append(self.migration_message)

                if self.migration_doc_url:
                    event['migration_doc_url'] = self.migration_doc_url

            event['message'] = '\n'.join(lines).format(**event)

            result.append(event)

        result.sort(key=lambda event: event['start_datetime'])

        return result


class EventsHandler(JCAPIHandler):
    @require_server
    async def get(self, name: str):
        vm = self.user_from_username(name).spawner.vm

        events = EventsConfigurable(
            config=self.settings['config'],
            username=name,
            vm=vm,
        )

        data = await events.get()
        first = (
            min(event['start_datetime'] for event in data)
            if data else
            None
        )
        last = (
            max(event['start_datetime'] for event in data)
            if data else
            None
        )

        self.write({
            'events': data,
            'first': first,
            'last': last,
        })


class AnonymousEventsHandler(JCPageHandler):
    @web.authenticated
    async def get(self):
        if self.current_user is None:
            raise web.HTTPError(403)

        next_url = self.get_next_url(
            default=f'/api/events/{self.current_user.name}',
        )
        self.redirect(next_url)


default_handlers = [
    ('/api/events', AnonymousEventsHandler),
    (f'/api/events/{NAME_RE}', EventsHandler),
]
