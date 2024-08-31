import asyncio
import datetime
import itertools
import json
import urllib.parse
from dataclasses import asdict, dataclass
from typing import Any, Dict, List, Optional

from traitlets import Bool, Unicode
from traitlets.config import SingletonConfigurable

from .http import AsyncHTTPClientMixin, HTTPResponse


@dataclass(frozen=True)
class InfraFilter:
    service_id: int
    environment_id: int

    from_: Optional[datetime.datetime] = None
    to: Optional[datetime.datetime] = None
    duration: Optional[int] = None

    type_: Optional[str] = None
    severity: Optional[str] = None
    dc: Optional[List[str]] = None

    def __post_init__(self):
        assert self.to or self.duration
        # if there are not .from_, we use now()
        # but if there are not .to or .duration, we can't do anything
        # and I don't wanna set default to duration


class InfraClient(SingletonConfigurable, AsyncHTTPClientMixin):
    oauth_token = Unicode(config=True)
    url = Unicode('https://infra-api.yandex-team.ru/v1', config=True)
    link_base = Unicode('https://infra.yandex-team.ru', config=True)
    validate_cert = Bool(True, config=True)

    async def request(self, uri: str, **kwargs) -> HTTPResponse:
        url = self.url + '/' + uri.lstrip('/')

        return await self._raw_request(url=url, **kwargs)

    def get_event_url(self, event_id):
        return f'{self.link_base}/event/{event_id}'

    async def _get_events(
        self,
        *,
        service_id: int,
        environment_id: int,
        from_: datetime.datetime,
        to: Optional[datetime.datetime] = None,
        type_: Optional[str] = None,
        duration: Optional[int] = None,
        severity: Optional[str] = None,
        dc: Optional[List[str]] = None,
        limit: int = 10,  # TODO: think about working with limits
        offset: Optional[int] = None,
        do_retries: bool = False,
    ) -> Dict[str, Any]:
        from_ = from_ or datetime.datetime.now()
        to = to or from_ + datetime.timedelta(days=duration)

        query_dict = {
            'serviceId': service_id,
            'environmentId': environment_id,
            'from': int(from_.timestamp()),
            'to': int(to.timestamp()),
            'type': type_,
            'severity': severity,
            'dc': dc,
            'limit': limit,
            'offset': offset,
        }

        query_dict = {
            key: value
            for key, value in query_dict.items()
            if value is not None
        }

        query: str = urllib.parse.urlencode(query_dict, doseq=True)

        response: HTTPResponse = await self.request(
            uri=f'events?{query}',
            method='GET',
            do_retries=do_retries,
        )

        return json.loads(response.body)

    async def get_events(self, *filters: InfraFilter):
        coroutines = []
        for filter in filters:
            data = asdict(filter)

            coro = self._get_events(**data)  # todo: think about limits
            coroutines.append(coro)

        raw_result = await asyncio.gather(*coroutines)

        return tuple(itertools.chain(*raw_result))
