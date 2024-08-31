from aiohttp import web
from alice.paskills.penguinarium.views.nodes import AddNodeHandler, RemNodeHandler
from alice.paskills.penguinarium.views.intent import GetIntentsHandler
from alice.paskills.penguinarium.views.misc import solomon_handler, ping_handler
from alice.paskills.penguinarium.views.graph import (
    AddGraphHandler, GetGraphIntent, RemGraphHandler
)


def setup_routes(app: web.Application) -> None:
    app.router.add_routes([
        web.post('/add_node', AddNodeHandler().handle),
        web.post('/rem_node', RemNodeHandler().handle),
        web.post('/get_intents', GetIntentsHandler().handle),

        web.get('/solomon', solomon_handler),
        web.get('/ping', ping_handler),

        web.post('/add_graph', AddGraphHandler().handle),
        web.post('/get_graph_intent', GetGraphIntent().handle),
        web.post('/rem_graph', RemGraphHandler().handle)
    ])
