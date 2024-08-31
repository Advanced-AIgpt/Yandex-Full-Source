from abc import ABC, abstractmethod
from typing import Generator

from alice.beggins.cmd.manifestator.internal.model import DataEntry


class Parser(ABC):
    def parse(self, sequence: Generator[dict, None, None]) -> Generator[DataEntry, None, None]:
        for item in sequence:
            if self.should_skip_item(item):
                continue
            self.validate_item(item)
            yield self.convert_item(item)

    @abstractmethod
    def should_skip_item(self, item: dict) -> bool:
        """
        For item skipping.
        For example, some data sources can contain items with null values.
        It can mean meta information.
        :param item:
        :return:
        """
        raise NotImplementedError()

    @abstractmethod
    def validate_item(self, item: dict):
        """
        For item validation.
        For example, some data sources can contain items with unsupported values.
        :param item:
        :return:
        """
        raise NotImplementedError()

    @abstractmethod
    def convert_item(self, item: dict) -> DataEntry:
        """
        Convert item to `DataEntry` type
        :param item:
        :return:
        """
        raise NotImplementedError()


class StandardParser(Parser):
    def __init__(self, text_key: str = None, target_key: str = None, source: str = None):
        self._source = source
        self._text_key = text_key or 'text'
        self._target_key = target_key or 'target'

    def should_skip_item(self, item: dict) -> bool:
        return item[self._text_key] is None or item[self._target_key] is None

    def validate_item(self, item: dict):
        target = item[self._target_key]
        if target not in (0, 1):
            raise ValueError(f'`{target}` neither is 0 nor 1')

    def convert_item(self, item: dict) -> DataEntry:
        text = item[self._text_key].strip()
        target = item[self._target_key]
        source = self._source or item.get('source') or ''
        return DataEntry(text, target, source)


class UnmarkedParser(Parser):
    def __init__(self, target: int, text_key: str = None, source: str = None):
        assert target in (0, 1), f'the specified target is {target}'

        self._source = source
        self._text_key = text_key or 'text'
        self._target = target

    def should_skip_item(self, item: dict) -> bool:
        return item[self._text_key] is None

    def validate_item(self, item: dict):
        pass

    def convert_item(self, item: dict) -> DataEntry:
        text = item[self._text_key].strip()
        target = self._target
        source = self._source or item.get('source') or ''
        return DataEntry(text, target, source)


class AnalyticsGeneralParser(Parser):
    def __init__(self, text_key: str = None, target_key: str = None, source: str = None):
        self._source = source
        self._text_key = text_key or 'utterance'
        self._target_key = target_key or 'is_positive'

    def should_skip_item(self, item: dict) -> bool:
        return item[self._text_key] is None or item[self._target_key] is None

    def validate_item(self, item: dict):
        target = item[self._target_key]
        if target not in ('N', 'Y'):
            raise ValueError(f'`{target}` neither is N nor Y')

    def convert_item(self, item: dict) -> DataEntry:
        text = item[self._text_key].strip()
        target = 1 if item[self._target_key] == 'Y' else 0
        source = self._source or item.get('source') or ''
        return DataEntry(text, target, source)


class AnalyticsBasketParser(Parser):
    def __init__(self, source: str = None):
        self._source = source
        self._text_key = 'text'
        self._target_key = 'is_negative_query'

    def should_skip_item(self, item: dict) -> bool:
        return item[self._text_key] is None or item[self._target_key] is None

    def validate_item(self, item: dict):
        target = item[self._target_key]
        if target not in (0, 1):
            raise ValueError(f'`{target}` neither is 0 nor 1')

    def convert_item(self, item: dict) -> DataEntry:
        text = item[self._text_key].strip()
        target = 1 - item[self._target_key]
        source = self._source or item.get('source') or item.get('query_source') or ''
        return DataEntry(text, target, source)
