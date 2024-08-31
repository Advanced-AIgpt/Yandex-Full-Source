from alice.uniproxy.library.events.directive import Directive
from alice.uniproxy.library.events.event_exception import EventException


class GoAway(EventException):
    def __init__(self, event_id, message=None):
        self.message = message or "Bad streamId in Event %s" % (event_id,)
        self.initial_exception = None
        Directive.__init__(
            self,
            "System",
            "GoAway",
            {},
            event_id
        )
        Exception.__init__(self, self.message)
