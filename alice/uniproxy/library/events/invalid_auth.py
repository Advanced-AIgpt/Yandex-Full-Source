from alice.uniproxy.library.events.directive import Directive
from alice.uniproxy.library.events.event_exception import EventException


class InvalidAuth(EventException):
    def __init__(self, message, event_id):
        self.message = "Bad authorization: %s" % (message,)
        Directive.__init__(
            self,
            "System",
            "InvalidAuth",
            {},
            event_id
        )
        Exception.__init__(self, self.message)
