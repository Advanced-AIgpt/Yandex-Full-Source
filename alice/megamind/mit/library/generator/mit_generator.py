from typing import List, Tuple, Set

from alice.library.python.eventlog_wrapper.eventlog_wrapper import (
    split_raw_response_to_http_response_and_eventlog,
    parse_eventlog,
    ApphostEventlogWrapper,
)
from alice.megamind.mit.library.graphs_util import GraphsInfo
from alice.megamind.mit.library.requester import MegamindRequester, Query
from alice.megamind.mit.library.response import ResponseWrapper
from alice.megamind.mit.library.stubber import StubberRepository


GENERATOR_CGI_PARAMS: List[Tuple[str, str]] = [
    ('dump', 'eventlog'),
    ('dump_source_responses', '1'),
    ('eventlog_format', 'json'),
]


class MitGenerator:

    def __init__(self, megamind_requester: MegamindRequester, stubber_repository: StubberRepository,
                 graphs_info: GraphsInfo, megamind_nodes: Set[str]):
        self._requester = megamind_requester
        self._stubber_repository = stubber_repository
        self._megamind_nodes = megamind_nodes
        self._graphs_info = graphs_info

    def _filter_eventlog(self, eventlog_wrapper: ApphostEventlogWrapper) -> ApphostEventlogWrapper:
        records = filter(lambda r: r.get_source_name() in self._graphs_info.nodes and
                                   r.get_source_name() not in self._megamind_nodes,
                         eventlog_wrapper.records)
        return ApphostEventlogWrapper(list(records))

    def __call__(self, query: Query) -> ResponseWrapper:
        request_id: str = self._requester.get_current_request_id()
        _, response = self._requester.request_megamind(query, aux_cgi=GENERATOR_CGI_PARAMS)
        http_response, eventlog = split_raw_response_to_http_response_and_eventlog(
            response)

        assert isinstance(eventlog, list), f'Eventlog should be list, got {type(eventlog)}'
        self._stubber_repository.save_eventlog(self._requester.test_name, request_id,
                                               self._filter_eventlog(parse_eventlog(eventlog)))
        return ResponseWrapper(http_response)
