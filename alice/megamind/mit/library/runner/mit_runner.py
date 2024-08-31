import json

from alice.megamind.mit.library.requester import MegamindRequester, Query
from alice.megamind.mit.library.response import ResponseWrapper


class MitRunner:

    def __init__(self, megamind_requester: MegamindRequester):
        self._requester = megamind_requester

    def __call__(self, query: Query) -> ResponseWrapper:
        _, response = self._requester.request_megamind(query)
        return ResponseWrapper(json.loads(response))
