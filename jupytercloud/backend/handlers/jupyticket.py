import functools

import yarl

from tornado import web

from jupytercloud.backend.jupyticket import JupyTicket
from jupytercloud.backend.lib.util.format import pretty_json

from .arcadia import ArcadiaHandlerMixin
from .base import JCAPIHandler, JCPageHandler
from .utils import json_body


def jupyticket_from_id(method):
    @functools.wraps(method)
    def m(self, id, *args, **kwargs):
        jupyticket = JupyTicket.get(
            db=self.jupyter_cloud_db,
            startrek_client=self.startrek_client,
            id=id,
        )
        if not jupyticket:
            raise web.HTTPError(404, f'no such jupyticket with id {id}')

        kwargs['jupyticket'] = jupyticket
        return method(self, *args, **kwargs)

    return m


class JupyTicketRenderMixin(ArcadiaHandlerMixin):
    def get_jupyticket_render_info(self, jupyticket):
        shares = [m.as_dict() for m in jupyticket.get_all_shares()]
        startrek = [m.as_dict() for m in jupyticket.get_all_startrek_tickets()]
        nirvana = [m.as_dict() for m in jupyticket.get_all_nirvana_instances()]

        for share in shares:
            share['link'] = self.arcadia.get_arcadia_link(
                path=share['path'],
                revision=share['revision']
            )

        return {
            'jupyticket': jupyticket.as_dict() | {
                'arcadia': shares,
                'startrek': startrek,
                'nirvana': nirvana,
            },
            'startrek_front': self.startrek_client.front_url,
            'arcanum_front': self.arcadia.base_url,
        }


class JupyTicketHandler(JCAPIHandler, JupyTicketRenderMixin):
    def check_referer(self):
        referer = self.request.headers.get('Referer')
        referer_path = yarl.URL(referer).host

        if self.startrek_client.api_url.host == referer_path:
            return True

        return super().check_referer()

    @jupyticket_from_id
    async def get(self, jupyticket):
        model = jupyticket.model

        updated = model.created.isoformat() + '+0000'

        self.write({
            'key': jupyticket.id,
            'summary': model.title,
            'assignee': model.user_name,
            'updated': updated,
        })

    @web.authenticated
    @jupyticket_from_id
    @json_body
    async def post(self, jupyticket, json_data):
        data_to_update = {}
        for field in JupyTicket._fields:
            if value := json_data.pop(field, None):
                data_to_update[field] = value

        if json_data:
            raise web.HTTPError(400, f'failed to updated next fields: {list(json_data)}')

        jupyticket.update(**data_to_update)
        # return only updated data!
        updated_data = {k: getattr(jupyticket.model, k) for k in data_to_update}

        self.write(pretty_json({
            'jupyticket': updated_data,
        }))


class JupyTicketPageHandler(JCPageHandler, JupyTicketRenderMixin):
    @web.authenticated
    @jupyticket_from_id
    async def get(self, jupyticket):
        jcdata = self.get_jupyticket_render_info(jupyticket)
        html = await self.render_template(
            'jupyticket.html',
            jcdata=pretty_json(jcdata),
        )
        self.write(html)


default_handlers = [
    (r'/jupyticket', JupyTicketPageHandler),
    (r'/jupyticket/(\d+)', JupyTicketPageHandler),
    (r'/api/jupyticket/(\d+)', JupyTicketHandler),
]
