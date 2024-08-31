import typing

import tvmauth
from tornado.httputil import HTTPServerRequest
from traitlets import Bool, Instance, Integer, Unicode
from traitlets.config import SingletonConfigurable

from jupytercloud.backend.lib.util.exc import JupyterCloudException


SERVICE_TICKET_HEADER: str = 'X-Ya-Service-Ticket'


class TVMError(JupyterCloudException):
    pass


class TVMClient(SingletonConfigurable):
    production = Bool(True, config=True)
    self_alias = Unicode('jupytercloud', config=True)
    self_id = Integer(None, allow_none=True, config=True)
    auth_token = Unicode(None, allow_none=True, config=True)
    port = Integer(None, allow_none=True, config=True)
    client = Instance(tvmauth.TvmClient, allow_none=True)

    def start(self) -> None:
        """Initialize TVM client.

        A no-op when the client is already started.
        """
        if self.client is None:
            self.client = tvmauth.TvmClient(
                tvmauth.TvmToolClientSettings(
                    self_alias=self.self_alias,
                    auth_token=self.auth_token,
                    port=self.port,
                ),
            )

    def verify_request(self, request: HTTPServerRequest, whitelist: typing.Iterable[int]) -> None:
        """Check if request is signed with a right TVM service ticket.

        The ticket should be from one of the TVM service IDs in a whitelist.
        Throw TVMError otherwise.
        """
        self.start()
        service_ticket: str = request.headers.get(SERVICE_TICKET_HEADER)
        if not service_ticket:
            raise TVMError(f'No TVM service ticket ({SERVICE_TICKET_HEADER}: {service_ticket})')

        st: tvmauth.ServiceTicket = self.client.check_service_ticket(service_ticket)
        if st.src not in whitelist:
            raise TVMError(f'Source {st.src} not in whitelist {whitelist}')

    def get_service_ticket(self, dst_alias: str) -> str:
        """Return TVM service ticket for a destination service alias."""
        self.start()
        return self.client.get_service_ticket_for(dst_alias)

    def get_service_ticket_header(self, dst_alias: str) -> typing.Dict[str, str]:
        """Return service ticket header for a a destination service alias."""
        return {SERVICE_TICKET_HEADER: self.get_service_ticket(dst_alias)}
