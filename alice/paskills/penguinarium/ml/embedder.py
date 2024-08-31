from abc import ABC, abstractmethod
from typing import List
import logging
import numpy as np
from cachetools import LFUCache
from alice.paskills.penguinarium.dssm_applier import DssmApplier

logger = logging.getLogger(__name__)


class BaseEmbedder(ABC):
    @abstractmethod
    def embed(self, text: str) -> np.array:
        pass


class DssmEmbedder(BaseEmbedder):
    def __init__(
        self,
        path: str,
        input_name: str,
        output_name: str,
        empty_inputs: List[str] = None
    ) -> None:
        super().__init__()
        logger.info('Loading DSSM model...')
        self._model = DssmApplier(path.encode('utf-8'))
        logger.info('DSSM model loaded!')

        self._input_name = input_name.encode('utf-8')
        self._output_name = output_name.encode('utf-8')

        self._empty_inputs = empty_inputs or []
        self._empty_inputs = [ei.encode('utf-8') for ei in self._empty_inputs]

    def embed(self, text: str) -> np.array:
        record_s = {self._input_name: text.encode('utf-8')}
        for ei in self._empty_inputs:
            record_s[ei] = b''

        return np.array(self._model.predict(
            record_s=record_s,
            output_variable=self._output_name
        ), dtype=np.float32)


class CachedDssmEmbedder(DssmEmbedder):
    def __init__(self, cache_size: int, **kwargs) -> None:
        super().__init__(**kwargs)
        self._cache = LFUCache(maxsize=cache_size)

    def embed(self, text: str) -> np.array:
        if text in self._cache:
            return self._cache[text]

        embedding = super().embed(text)
        self._cache[text] = embedding

        return embedding
