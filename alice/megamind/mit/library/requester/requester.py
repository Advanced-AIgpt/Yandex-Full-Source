import logging
import requests
import socket
from typing import List, Tuple, Dict, Union
from urllib.parse import urlparse, urlunparse, urlencode
import uuid


from alice.library.python.testing.megamind_request.input_dialog import AbstractInput
from alice.megamind.mit.library.request_builder import MegamindRequestBuilder
from alice.megamind.mit.library.stubber import ApphostStubberService
from alice.tests.library.service import AppHost, Megamind


CGI_PARAMS: List[Tuple[str, str]] = [
    ('waitall', 'da'),
    ('timeout', '1000000'),
]


logger = logging.getLogger(__name__)


Query = Union[MegamindRequestBuilder, AbstractInput]


class MegamindRequester:

    def __init__(self, megamind: Megamind, apphost: AppHost, stubber: ApphostStubberService, test_name: str, experiments: Dict[str, str]):
        apphost.wait_port()
        self._megamind = megamind
        self._apphost = apphost
        self._stubber = stubber
        self._test_name = test_name
        self._request_counter = 0
        self._experiments = experiments

    @property
    def test_name(self) -> str:
        return self._test_name

    def get_current_request_id(self) -> str:
        return str(uuid.uuid5(uuid.NAMESPACE_OID,
                              f'{self._test_name},{self._request_counter}'))

    def request_megamind(self, query: Query,
                         aux_cgi: List[Tuple[str, str]] = []) -> Tuple[int, str]:
        builder = query if isinstance(query, MegamindRequestBuilder) else MegamindRequestBuilder(query)
        builder.set_request_id(self.get_current_request_id())
        builder.add_experiments(self._experiments)
        skr_json = builder.build()

        all_srcrwr: List[str] = self._stubber.srcrwrs
        for node_name in self._stubber.get_megamind_unstubbed_nodes():
            all_srcrwr.append(f'{node_name}:{socket.gethostname()}:{self._megamind.grpc_port}')
        all_cgi_params = CGI_PARAMS + aux_cgi + \
            [('srcrwr', srcrwr) for srcrwr in all_srcrwr]
        handle_suffix = 'apply/' if builder.is_apply else 'app/pa/'
        url_parts = list(
            urlparse(f'http://localhost:{self._apphost.http_adapter_port}/speechkit/{handle_suffix}'))
        url_parts[4] = urlencode(all_cgi_params)
        url = urlunparse(url_parts)
        logger.info(f'Megamind url: {url}')

        headers = {'content-type': 'application/json',
                   'Accept-Charset': 'UTF-8', 'X-Yandex-Internal-Request': '1'}
        response = requests.post(url, json=skr_json, headers=headers)

        logger.info(
            f'Request to Url: {url}, returned Code: {response.status_code}')
        self._request_counter += 1
        logger.info(f'Megamind response.text: {response.text}')
        return response.status_code, response.text
