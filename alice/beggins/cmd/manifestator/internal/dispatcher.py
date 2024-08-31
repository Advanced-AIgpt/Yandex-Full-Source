import random
from abc import ABC, abstractmethod
from typing import List, Generator

from alice.beggins.cmd.manifestator.internal.model import DataEntry


class Dispatcher(ABC):
    @abstractmethod
    def dispatch(self, sequence: Generator[DataEntry, None, None]) -> Generator[DataEntry, None, None]:
        raise NotImplementedError()


class Chain(Dispatcher):
    def __init__(self, dispatchers: List[Dispatcher]):
        self._dispatchers = dispatchers

    def dispatch(self, sequence: Generator[DataEntry, None, None]) -> Generator[DataEntry, None, None]:
        for dispatcher in self._dispatchers:
            sequence = dispatcher.dispatch(sequence)
        for item in sequence:
            yield item


class PositivesNegativesFilter(Dispatcher):
    def __init__(self, target):
        self._target = target

    def dispatch(self, sequence: Generator[DataEntry, None, None]) -> Generator[DataEntry, None, None]:
        for item in sequence:
            if item.target == self._target:
                yield item


class NegativesFilter(PositivesNegativesFilter):
    def __init__(self):
        super().__init__(target=0)


class PositivesFilter(PositivesNegativesFilter):
    def __init__(self):
        super().__init__(target=1)


class InvertModifier(Dispatcher):
    def dispatch(self, sequence: Generator[DataEntry, None, None]) -> Generator[DataEntry, None, None]:
        for item in sequence:
            yield DataEntry(
                text=item.text,
                target=1 - item.target,
                source=item.source,
            )


class EntriesLimiter(Dispatcher):
    def __init__(self, limit: int):
        self._limit = limit
        self._count = 0

    def dispatch(self, sequence: Generator[DataEntry, None, None]) -> Generator[DataEntry, None, None]:
        for item in sequence:
            yield item
            self._count += 1
            if self._count == self._limit:
                break


class Identity(Dispatcher):
    def dispatch(self, sequence: Generator[DataEntry, None, None]) -> Generator[DataEntry, None, None]:
        for item in sequence:
            yield item


class RandomLimiter(Dispatcher):
    def __init__(self, threshold: float, rnd: random.Random):
        self._threshold = threshold
        self._rnd = rnd

    def dispatch(self, sequence: Generator[DataEntry, None, None]) -> Generator[DataEntry, None, None]:
        for item in sequence:
            if self._rnd.random() > self._threshold:
                continue
            yield item
