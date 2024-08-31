import logging

from alice.uniproxy.library.events.directive import Directive


class EventException(Directive, Exception):
    def __init__(self, message, event_id=None, initial_exception=None):
        self.message = message if isinstance(message, dict) else str(message)
        self.initial_exception = initial_exception
        logging.warning("Client will get exception: %s because of %s" % (message, initial_exception or "reasons"))
        if initial_exception:
            logging.exception(initial_exception)

        super(EventException, self).__init__(
            "System",
            "EventException",
            {
                "error": {
                    "type": "Error",
                    "message": self.message
                }
            },
            event_id
        )


class EventExceptionEx(Directive, Exception):
    def __init__(self, message, details=None, event_id=None, initial_exception=None, response={}):
        self.message = str(message)
        self.initial_exception = initial_exception
        logging.warning("Client will get exception: %s because of %s" % (message, initial_exception or "reasons"))
        if initial_exception:
            logging.exception(initial_exception)

        payload = {
            "error": {
                "type": "Error",
                "message": self.message,
            },
        }

        if details:
            payload.update({
                'details': details
            })

        if response:
            payload.update({
                'Response': response
            })

        super(EventExceptionEx, self).__init__(
            "System",
            "EventException",
            payload,
            event_id
        )
