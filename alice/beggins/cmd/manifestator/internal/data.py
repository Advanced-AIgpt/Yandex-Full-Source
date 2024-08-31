from abc import ABC, abstractmethod
from typing import Generator

from yt.wrapper import YtClient

import alice.beggins.cmd.manifestator.internal.model as model
from alice.beggins.cmd.manifestator.internal.dispatcher import Dispatcher
from alice.beggins.cmd.manifestator.internal.parser import Parser


class DataSource(ABC):
    def __init__(self, parser: Parser, dispatcher: Dispatcher):
        self._parser = parser
        self._dispatcher = dispatcher

    def get_entries(self) -> Generator[model.DataEntry, None, None]:
        for row in self._dispatcher.dispatch(self._parser.parse(self._get_entries_impl())):
            yield row

    @abstractmethod
    def size(self):
        raise NotImplementedError()

    @abstractmethod
    def _get_entries_impl(self) -> Generator[model.DataEntry, None, None]:
        raise NotImplementedError()


class YtDataSource(DataSource):
    def __init__(self, table: str, dispatcher: Dispatcher, parser: Parser, proxy: str = None):
        super().__init__(parser, dispatcher)
        self._table = table
        self._proxy = proxy or 'hahn'
        self._yt_client = YtClient(proxy=self._proxy)

    def size(self):
        return self._yt_client.row_count(self._table)

    def _get_entries_impl(self) -> Generator[model.DataEntry, None, None]:
        for row in self._yt_client.read_table(self._table):
            yield row


class LocalFileDataSource(DataSource):
    def __init__(self, filepath: str, dispatcher: Dispatcher, parser: Parser):
        super().__init__(parser, dispatcher)
        self._filepath = filepath

    def size(self):
        n_lines = 0
        with open(self._filepath, 'r') as stream:
            for _ in stream:
                n_lines += 1
        return n_lines

    def _get_entries_impl(self) -> Generator[model.DataEntry, None, None]:
        with open(self._filepath, 'r') as stream:
            for row in stream:
                yield {'text': row.strip()}
