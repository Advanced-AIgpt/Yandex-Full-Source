import json
from alice.uniproxy.library.logging import Logger


class WebHandlerLogger:
    def __init__(self):
        self._log = Logger.get(".webhandler")

    def __call__(self, message):
        self._log.info(f"WEBHANDLER: {json.dumps(message)}")
