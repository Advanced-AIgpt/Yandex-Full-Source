import asyncio
import re

from tornado import web

from .base import JCAPIHandler
from .utils import json_body


TICKET_RE = re.compile(r'\w+-\d+')


class StartrekValidateTicketsHandler(JCAPIHandler):
    post_schema = {
        'type': 'object',
        'properties': {
            'tickets': {
                'type': 'array',
                'items': {
                    'type': 'string',
                },
                'minItems': 1,
                'uniqueItems': True,
            },
        },
        'required': ['tickets'],
    }

    def raise_on_bad_tickets(self, tickets, bad_tickets, message):
        if not bad_tickets:
            return

        repr = ', '.join(bad_tickets)
        prefix = "next tickets don't " if len(tickets) > 1 else "ticket doesn't "
        suffix = f': {repr}'

        raise web.HTTPError(400, f'{prefix}{message}{suffix}')

    @json_body
    async def post(self, json_data):
        good_tickets = []
        bad_tickets = []
        tickets = json_data['tickets']

        for ticket in tickets:
            ticket = ticket.removeprefix(self.startrek_client.front_url).strip('/')
            if TICKET_RE.fullmatch(ticket):
                good_tickets.append(ticket)
            else:
                bad_tickets.append(ticket)

        self.raise_on_bad_tickets(tickets, bad_tickets, 'match ticket format')

        coros = [self.startrek_client.get_ticket(ticket) for ticket in good_tickets]
        tickets_info = await asyncio.gather(*coros)

        for ticket, info in zip(good_tickets, tickets_info):
            if info is None:
                bad_tickets.append(ticket)

        self.raise_on_bad_tickets(tickets, bad_tickets, 'exists in startrek')

        self.write({
            'tickets': good_tickets,
        })


class StartrekExtractTicketsHandler(JCAPIHandler):
    post_schema = {
        'type': 'object',
        'properties': {
            'notebook_path': {
                'type': 'string',
            },
        },
        'required': ['notebook_path'],
    }

    @json_body
    async def post(self, json_data):
        notebook_path = json_data['notebook_path']

        raw_tickets = TICKET_RE.findall(notebook_path)

        coros = [self.startrek_client.get_ticket(ticket) for ticket in raw_tickets]
        tickets_info = await asyncio.gather(*coros)

        tickets = []

        for ticket, info in zip(raw_tickets, tickets_info):
            if info is not None:
                tickets.append(ticket)

        self.write({
            'tickets': tickets,
        })


class StartrekGetTicketsInfo(JCAPIHandler):
    @web.authenticated
    async def get(self):
        tickets = self.get_query_arguments('ticket_id')

        if not tickets:
            raise web.HTTPError(400, 'missing ticket_id query argument')

        # NB: on 50 tickets startrek turns on pagination
        # I don't plan to fix it, because 50 tickets is enough for everyone
        if len(tickets) >= 50:
            raise web.HTTPError(400, 'too much tickets')

        data = await self.startrek_client.get_tickets(tickets)

        result = {}

        for ticket_info in data:
            result[ticket_info['key']] = ticket_info

        for key in tickets:
            if key not in result:
                result[key] = None

        self.write({'tickets': result})


default_handlers = [
    ('/api/statrek/validate_tickets', StartrekValidateTicketsHandler),
    ('/api/statrek/extract_tickets', StartrekExtractTicketsHandler),
    ('/api/statrek/get_tickets_info', StartrekGetTicketsInfo),
]
