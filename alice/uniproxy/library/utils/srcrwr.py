from typing import List

import tornado.httputil


class Srcrwr:

    NAME = 'x-srcrwr'

    CGI = 'srcrwr'

    SEP = ';'

    def __init__(self, headers: tornado.httputil.HTTPHeaders, query_arguments: List = None):
        self.ports = {}
        self._header = header = self._normalize_header(headers.get(self.NAME))
        self._query_argument = query_argument = self._normalize_query_argument(query_arguments)
        self._rewrites = self._parse_rewrites(header, query_argument)

    def __getitem__(self, source):
        return self._rewrites.get(source)

    @property
    def header(self) -> dict:
        if self._rewrites:
            return {
                self.NAME: ';'.join(
                    '%s=%s' % (k, v) for k, v in self._rewrites.items()
                )
            }
        return {}

    def _normalize_header(self, header):
        if header is not None:
            return header.replace(',', self.SEP)

    def _normalize_query_argument(self, query_argument):
        if query_argument:
            return self.SEP.join(query_argument).replace(',', self.SEP)

    def _parse_url(self, service, url):
        pos = url.rfind(':')
        if (pos != -1) and (pos >= len(url) - 6):
            host, port = url[:pos], url[pos + 1:]
            self.ports[service] = port
            return host
        return url

    def _parse_rewrites(self, header, query_argument):
        rewrites = {}

        if header:
            for srcrwr in header.split(self.SEP):
                service, url = srcrwr.split('=', 1)
                rewrites[service] = self._parse_url(service, url)

        if query_argument:
            for srcrwr in query_argument.split(self.SEP):
                service, url = srcrwr.split(':', 1)
                rewrites[service] = self._parse_url(service, url)

        return rewrites

    def as_dict(self):
        return self._rewrites
