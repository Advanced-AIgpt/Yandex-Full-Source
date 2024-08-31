from abc import ABC, abstractmethod
from typing import List, Dict, Any
from collections import namedtuple
import numpy as np

from alice.paskills.penguinarium.ml.embedder import DssmEmbedder
from alice.paskills.penguinarium.ml.index import ModelFactory, BaseIndex
from alice.paskills.penguinarium.util.metrics import sensors

IResolveRes = namedtuple('IResolveRes', ['distances', 'preds'])


class BaseIntentResolver(ABC):
    def __init__(self, model_factory: ModelFactory) -> None:
        self._model_factory = model_factory

    @abstractmethod
    def build_model(self, utterances: List[str], model_params: Dict = None) -> Any:
        pass

    @abstractmethod
    def resolve_intents(self, utterance: List[str], model) -> IResolveRes:
        pass


class DssmKnnIntentResolver(BaseIntentResolver):
    def __init__(self, embedder: DssmEmbedder, model_factory: ModelFactory) -> None:
        super().__init__(model_factory=model_factory)
        self._embedder = embedder

    def build_model(
        self,
        utterances: List[str],
        intents: List[str],
        model_params: Dict = None
    ) -> Any:
        model_params = model_params or {}

        embeddings = np.array(
            [self._embedder.embed(u) for u in utterances],
            dtype=np.float32
        )
        knn_index = self._model_factory.produce(**model_params)
        knn_index.build(embeddings=embeddings, intents=intents)

        return knn_index

    @sensors.with_timer('resolve_intents_time')
    async def resolve_intents(
        self,
        utterance: List[str],
        model: BaseIndex
    ) -> IResolveRes:
        embedding = self._embedder.embed(utterance)
        search_res = model.search(embedding)

        return IResolveRes(
            distances=search_res.distances.tolist(),
            preds=search_res.preds.tolist()
        )
