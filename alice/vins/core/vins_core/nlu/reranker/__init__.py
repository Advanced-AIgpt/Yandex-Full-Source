# -*- coding: utf-8 -*-

from vins_core.nlu.reranker.factors import (WordNNFactor, IntentBasedFactor, BagOfEntitiesFactor, DenseFeatureFactor,
                                            KNNFactor, FallbackFactor)
from vins_core.nlu.reranker.factor_calcer import register_factor_type

register_factor_type(WordNNFactor, WordNNFactor.NAME)
register_factor_type(KNNFactor, KNNFactor.NAME)
register_factor_type(FallbackFactor, FallbackFactor.NAME)
register_factor_type(IntentBasedFactor, IntentBasedFactor.NAME)
register_factor_type(BagOfEntitiesFactor, BagOfEntitiesFactor.NAME)
register_factor_type(DenseFeatureFactor, DenseFeatureFactor.NAME)
