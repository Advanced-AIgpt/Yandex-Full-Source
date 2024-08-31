from random import Random
from typing import List
import attr

from alice.beggins.cmd.manifestator.internal.data import DataSource


@attr.s()
class Dataset:
    name: str = attr.ib()
    sources: List[DataSource] = attr.ib()


@attr.s()
class Data:
    train: Dataset = attr.ib()
    accept: Dataset = attr.ib()


@attr.s()
class Manifest:
    data: Data = attr.ib()
    random: Random = attr.ib()
