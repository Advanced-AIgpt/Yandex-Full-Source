from alice.uniproxy.library.async_http_client import QueuedHTTPClient, HTTPRequest


class MdsResponse:
    def __init__(self, code, body):
        self.code = code
        self.body = body


class AsyncMdsClient:
    def __init__(
        self,
        tvm_client,  # PersonalTvmClient
        host="storage-int.mds.yandex.net",
        port=443,
        http_client_kwargs={},
    ):
        self.tvm_client = tvm_client
        self.host = host
        self.port = port

        http_client_kwargs.setdefault("secure", self.port == 443)

        self.http_client = QueuedHTTPClient.get_client(
            self.host,
            self.port,
            **http_client_kwargs,
        )

    async def get(self, namespace, path, cgi={}):
        query = f"/{namespace}/{path}"

        if cgi:
            query = query + '?' + '&'.join([f'{k}={v}' for (k, v) in cgi.items()])

        request = HTTPRequest(
            query=query,
            headers={
                "X-Ya-Service-Ticket": await self.tvm_client.get_service_ticket(),
                "Host": self.host,
            }
        )

        response = await self.http_client.fetch(request, raise_error=False)
        return MdsResponse(
            response.code,
            response.body,
        )
