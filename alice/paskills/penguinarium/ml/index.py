from abc import ABC, abstractmethod
from typing import Callable, Union, List, Type
import pickle
import json
from base64 import b64encode, b64decode
from collections import namedtuple

import numpy as np
from sklearn.neighbors import NearestNeighbors

SearchRes = namedtuple('SearchRes', ['distances', 'preds'])
SerializedModel = namedtuple('SerializedModel', ['meta', 'binary'])


class NotBuildedError(RuntimeError):
    pass


class BaseIndex(ABC):
    def __init__(
        self,
        thresh: float,
        dist_thresh_rel: Union[str, Callable],
        n_neighbors: int
    ) -> None:
        super().__init__()
        self._thresh = thresh
        self._dist_thresh_rel = dist_thresh_rel
        self._n_neighbors = n_neighbors
        self._builded = False

    def _top(self, seach_res: SearchRes) -> SearchRes:
        seen_preds = set()
        top_distances, top_preds = [], []
        preds = self._intents[seach_res.preds]

        for dist, pred in zip(seach_res.distances, preds):
            if not self._dist_thresh_rel(dist, self._thresh):
                break

            if pred not in seen_preds:
                top_distances.append(dist)
                top_preds.append(pred)

                seen_preds.add(pred)
                if len(seen_preds) >= self._n_neighbors:
                    break

        return SearchRes(
            distances=np.array(top_distances),
            preds=np.array(top_preds)
        )

    @abstractmethod
    def _build(self, embeddings: np.array, n_neighbors: int) -> None:
        pass

    def build(self, embeddings: np.array, intents: List[str]) -> None:
        self._embeddings = embeddings
        self._intents = np.array(intents)
        _, counts = np.unique(self._intents, return_counts=True)
        sorted_counts = counts[counts.argsort()[::-1]]
        n_neighbors = sorted_counts[:self._n_neighbors].sum()
        self._build(embeddings=embeddings, n_neighbors=n_neighbors)
        self._builded = True

    @property
    def embeddings(self) -> np.array:
        if not self._builded:
            raise NotBuildedError()
        return self._embeddings

    @property
    def intents(self) -> np.array:
        if not self._builded:
            raise NotBuildedError()
        return self._intents

    @abstractmethod
    def _search(self, embedding: np.array) -> SearchRes:
        pass

    def search(self, embedding: np.array) -> SearchRes:
        if not self._builded:
            raise NotBuildedError()

        search_res = self._search(embedding=embedding)
        return self._top(search_res)

    @abstractmethod
    def _serialize(self) -> SerializedModel:
        pass

    def serialize(self) -> SerializedModel:
        if not self._builded:
            raise NotBuildedError()
        return self._serialize()

    @classmethod
    @abstractmethod
    def load(cls, serialized: SerializedModel):
        pass


def bytes2str(bytes_buf: bytes) -> str:
    return b64encode(bytes_buf).decode('utf-8')


def str2bytes(str_buf: str) -> bytes:
    return b64decode(str_buf)


class SklearnIndex(BaseIndex):
    def __init__(
        self,
        thresh: float,
        dist_thresh_rel: Union[Callable, str],
        metric: Union[Callable, str],
        n_neighbors: int,
        p: int
    ) -> None:
        super().__init__(
            thresh=thresh,
            dist_thresh_rel=dist_thresh_rel,
            n_neighbors=n_neighbors
        )
        self._metric = metric
        self._p = p

    def _build(self, embeddings: np.array, n_neighbors: int) -> None:
        self._nn = NearestNeighbors(
            metric=self._metric,
            p=self._p,
            n_neighbors=n_neighbors,
            algorithm='brute',
            n_jobs=1,
        )
        self._nn.fit(X=embeddings)

    def _search(self, embedding: np.array) -> SearchRes:
        embedding = np.array(embedding).reshape(1, -1)
        search_res = self._nn.kneighbors(embedding)

        distances, idx = search_res

        return SearchRes(
            preds=idx.flatten(),
            distances=distances.flatten()
        )

    def _serialize(self) -> SerializedModel:
        meta = {
            'thresh': self._thresh,
            'dist_thresh_rel': bytes2str(pickle.dumps(self._dist_thresh_rel)),
            'metric': bytes2str(pickle.dumps(self._metric)),
            'n_neighbors': self._n_neighbors,
            'intents': self._intents.tolist(),
            'dim': self._embeddings.shape[1],
            'p': self._p
        }
        return SerializedModel(
            meta=json.dumps(meta),
            binary=self._embeddings.tobytes()
        )

    @classmethod
    def load(cls, serialized: SerializedModel) -> 'SklearnIndex':
        params = json.loads(serialized.meta)
        instance = cls(
            thresh=params['thresh'],
            dist_thresh_rel=pickle.loads(str2bytes(params['dist_thresh_rel'])),
            metric=pickle.loads(str2bytes(params['metric'])),
            n_neighbors=params['n_neighbors'],
            p=params['p']
        )

        embeddings = np.frombuffer(
            serialized.binary,
            dtype=np.float32
        ).reshape(-1, params['dim'])
        instance.build(
            embeddings=embeddings,
            intents=params['intents']
        )

        return instance


class ModelFactory:
    def __init__(self, model_cls: Type, **kwargs) -> None:
        self._model_cls = model_cls
        self._kwargs = kwargs

    def produce(self, **kwargs) -> Type:
        joint_kwargs = {**self._kwargs, **kwargs}
        return self._model_cls(**joint_kwargs)

    def load(self, serialized: SerializedModel) -> Type:
        return self._model_cls.load(serialized=serialized)
