# coding: utf-8

from nlu_service.web.handlers import (
    FaviconHandler,
    InflectHandler,
    PingHandler,
    RootHandler,
    NerApiHandler,
    UiHandler,
    UnistatHandler,
)

urls = [  # pylint: disable=invalid-name
    (r'/', RootHandler),
    (r'/ui', UiHandler),
    (r'/ping', PingHandler),
    (r'/unistat', UnistatHandler),
    (r'/favicon.ico', FaviconHandler),
    (r'/api/ner/v1', NerApiHandler),
    (r'/api/inflect/v1', InflectHandler),
]
