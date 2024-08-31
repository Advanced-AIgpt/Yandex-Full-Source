# -*- coding: utf-8 -*-

import attr
import os
import logging
import numpy as np
import scipy.sparse as sps
import cPickle as pickle

# Imports for typing
from typing import Mapping, Dict, List, Iterator, Set, Tuple, Union, AnyStr, Any  # noqa: UnusedImport
from typing import Container, NoReturn, Generator, Optional, Callable, Iterable  # noqa: UnusedImport

from enum import Enum
from collections import defaultdict, Counter
from itertools import izip, chain
from operator import attrgetter
from sklearn.model_selection import StratifiedShuffleSplit

from vins_core.nlu.base_nlu import FeatureExtractorResult
from vins_core.nlu.features.base import SampleFeatures
from vins_core.nlu.features.extractor.base import SparseFeatureValue
from vins_core.utils.data import TarArchive
from vins_core.dm.formats import NluSourceItem
from vins_core.common.sample import Sample


logger = logging.getLogger(__name__)


@attr.s
class AdditionalInfo(object):
    device_state = attr.ib()
    prev_intent = attr.ib()
    slots = attr.ib()
    raw_factors_data = attr.ib()
    entities_info = attr.ib(default=None)


class VinsDatasetView(object):
    """An immutable dataset classifier-specific representation.
    Stores all classifier's features and labels in parallel lists.
    Can be sliced: returns mapping from feature names to list of values, lengths of lists are equal to len of indices.
    """
    def __init__(self,
                 dataset_indices,  # type: List[int]
                 sample_ranges,  # type: List[Tuple[int, int]]
                 intents,  # type: List[AnyStr]
                 preprocessed_texts,  # type: List[AnyStr]
                 dense_embeddings,  # type: Dict[AnyStr, np.ndarray]
                 dense_seq_embeddings,  # type: Dict[AnyStr, np.ndarray]
                 sparse_embeddings,  # type: Dict[AnyStr, sps.csr_matrix]
                 features_indices,  # type: Dict[AnyStr, Dict[AnyStr, int]]
                 additional_infos,  # type: List[AdditionalInfo]
                 classifier_feature_mappings=None  # type: Dict[AnyStr, Dict[AnyStr, int]]
                 ):
        self._dataset_indices = dataset_indices
        self._sample_ranges = sample_ranges
        self._intents = intents
        self._preprocessed_texts = preprocessed_texts
        self._dense_embeddings = dense_embeddings
        self._dense_seq_embeddings = dense_seq_embeddings
        self._sparse_embeddings = sparse_embeddings
        self._features_indices = features_indices
        self._additional_infos = additional_infos
        self._classifier_feature_mappings = classifier_feature_mappings or {}

        self._possible_intents = []
        intent_to_index = {}
        for intent in self._intents:
            if intent not in intent_to_index:
                intent_to_index[intent] = len(intent_to_index)
                self._possible_intents.append(intent)

        self._intent_indices = np.array([intent_to_index[intent] for intent in self._intents])

    @property
    def possible_intents(self):
        # type: () -> List[AnyStr]
        return self._possible_intents

    def get_features(self):
        # type: () -> Dict[AnyStr, Optional[int]]

        def _add_features_info(embeddings):
            return {feat_type: feat_matrix.shape[-1] for feat_type, feat_matrix in embeddings.iteritems()}

        features = {}
        features.update(_add_features_info(self._dense_embeddings))
        features.update(_add_features_info(self._dense_seq_embeddings))
        features.update(_add_features_info(self._sparse_embeddings))
        if self._preprocessed_texts:
            features['texts'] = None

        return features

    def __len__(self):
        return len(self._sample_ranges)

    def __getitem__(self, indices):
        # type: (...) -> Dict[AnyStr, List[np.ndarray]]

        if isinstance(indices, int):
            indices = [indices]
        elif isinstance(indices, slice):
            indices = self._convert_slice_to_indices(indices, len(self._sample_ranges))

        assert isinstance(indices, (tuple, list, np.ndarray)), 'Unexpected type of slice'

        output = {}
        for feat_type, feat_matrix in self._dense_embeddings.iteritems():
            output[feat_type] = feat_matrix[indices].reshape((len(indices), feat_matrix.shape[-1]))

        output.update(self._slice_embeddings_by_indices(self._dense_seq_embeddings, indices))
        output.update(self._slice_embeddings_by_indices(self._sparse_embeddings, indices))

        if self._preprocessed_texts is not None:
            output['texts'] = [self._preprocessed_texts[ind] for ind in indices]

        if self._intent_indices is not None:
            output['intent'] = [self._intents[ind] for ind in indices]
        return output

    @staticmethod
    def _convert_slice_to_indices(slice_obj, length):
        # type: (slice, int) -> List

        start = 0 if not slice_obj.start or slice_obj.start < -length else slice_obj.start % length
        stop = length if not slice_obj.stop or slice_obj.stop > length else slice_obj.stop % length
        return range(start, stop, slice_obj.step or 1)

    def _slice_embeddings_by_indices(self, embeddings, indices):
        # type: (Union[np.ndarray, sps.csr_matrix], List[int]) -> Dict[AnyStr, List[Union[np.ndarray, sps.csr_matrix]]]

        return {
            feat_type: [feat_matrix[self._sample_ranges[ind][0]: self._sample_ranges[ind][1]] for ind in indices]
            for feat_type, feat_matrix in embeddings.iteritems()
        }

    def map_classifiers(self, source_classifier, target_classifier, renames=None):
        source_intent_to_idx = self._classifier_feature_mappings[source_classifier]
        if renames:
            source_intent_to_idx = {
                renames.get(intent, intent): idx for intent, idx in source_intent_to_idx.iteritems()
            }

        target_intent_to_idx = self._classifier_feature_mappings[target_classifier]

        return {
            target_idx: source_intent_to_idx[intent] for intent, target_idx in target_intent_to_idx.iteritems()
            if intent in source_intent_to_idx
        }

    def map_predictions_to_dataset(self, preds):
        # type: (np.ndarray) -> np.ndarray
        view_unique_indices, dataset_indices = np.unique(self._dataset_indices, return_index=True)
        assert np.all(view_unique_indices == np.arange(len(dataset_indices)))
        return preds[dataset_indices]


class DenseSeqSourceType(Enum):
    DENSE_SEQ = 1
    DENSE_SEQ_IDS = 2
    SPARSE_SEQ = 3


class VinsDataset(object):
    """An universal dataset representation.
    Stores all source items and their features in parallel lists.
    Additionally for each item a mapping from classifier to list of intents + some meta-information are stored.
    The dataset can be converted to SampleFeatures or VinsDatasetView to be used for model training.
    """

    # TODO: Sparse features are not supported currently
    class FeatureType(Enum):
        DENSE = 1
        DENSE_SEQ = 2
        SPARSE_SEQ = 3
        INDEX_SEQ = 4

    def __init__(self,
                 sample_ranges,  # type: List[Tuple[int, int]]
                 dense_embeddings,  # type: Dict[AnyStr, np.ndarray]
                 dense_seq_embeddings,  # type: Dict[AnyStr, np.ndarray]
                 sparse_embeddings,  # type: Dict[AnyStr, sps.csr_matrix]
                 nlu_source_items,  # type: List[NluSourceItem]
                 preprocessed_texts,  # type: List[AnyStr]
                 sample_tags,  # type: List[AnyStr]
                 weights,  # type: np.ndarray
                 intents,  # type: List[AnyStr]
                 classifier_lists,  # type: List[AnyStr]
                 dense_seq_source_types,  # type: Dict[AnyStr, DenseSeqSourceType]
                 sparse_feature_mappings,  # type: Dict[AnyStr, Dict[AnyStr, int]]
                 classifier_feature_mappings,  # type: Dict[AnyStr, Dict[AnyStr, int]]
                 additional_infos,  # type: List[AdditionalInfo]
                 ):
        self._sample_ranges = sample_ranges
        self._dense_embeddings = dense_embeddings
        self._dense_seq_embeddings = dense_seq_embeddings
        self._sparse_embeddings = sparse_embeddings
        self._nlu_source_items = nlu_source_items
        self._preprocessed_texts = preprocessed_texts
        self._sample_tags = sample_tags
        self._weights = weights
        self._classifier_lists = classifier_lists
        self._intents = intents
        self._dense_seq_source_types = dense_seq_source_types
        self._sparse_feature_mappings = sparse_feature_mappings
        self._classifier_feature_mappings = classifier_feature_mappings
        self._additional_infos = additional_infos

        self.build_feature_list()

    def __len__(self):
        return len(self._sample_ranges)

    def build_feature_list(self):
        self._sparse_feature_lists = {
            feat_type: [feat for feat, _ in sorted(feat_mapping.iteritems(), key=lambda x: x[1])]
            for feat_type, feat_mapping in self._sparse_feature_mappings.iteritems()
        }

    @property
    def features(self):
        return set(chain(self._dense_embeddings, self._dense_seq_embeddings, self._sparse_embeddings))

    def add_classifier_feature_mapping(self, classifier, feature_mapping):
        self._classifier_feature_mappings[classifier] = feature_mapping

    def convert_classifier_feature_by_renaming(self, source_feature_name, target_feature_name, renames):
        source_mapping = self._classifier_feature_mappings[source_feature_name]
        target_mapping = self._classifier_feature_mappings[target_feature_name]

        target_indices = [[] for _ in xrange(len(target_mapping))]
        for source_intent, source_index in source_mapping.iteritems():
            target_intent = renames(source_intent)
            target_index = target_mapping.get(target_intent, None)
            if target_index:
                target_indices[target_index].append(source_index)
            else:
                print target_intent

        source_feature = self._dense_embeddings[source_feature_name]
        target_feature = np.zeros((len(source_feature), len(target_indices)))
        for target_index, target_indices in enumerate(target_indices):
            target_feature[:, target_index] = source_feature[:, target_indices].sum(-1)

        self._dense_embeddings[source_feature_name + '_to_' + target_feature_name] = target_feature
        self._classifier_feature_mappings[source_feature_name + '_to_' + target_feature_name] = target_mapping

    def to_intent_to_sample_features(self, classifier):
        # type: (AnyStr) -> Dict[AnyStr, List[SampleFeatures]]

        converted_items = self._convert_items(classifier)

        result = defaultdict(list)
        for _, intent, sample_features in converted_items:
            result[intent].append(sample_features)
        return result

    def to_sample_features_list(self, classifier):
        # type: (AnyStr) -> List[SampleFeatures]

        converted_items = self._convert_items(classifier)
        return [sample_features for _, _, sample_features in converted_items]

    def convert_items_to_fer(self, classifier):
        # type: (AnyStr) -> Dict[AnyStr, List[FeatureExtractorResult]]

        converted_items = self._convert_items(classifier)
        result = defaultdict(list)
        for ind, intent, sample_features in converted_items:
            result[intent].append(FeatureExtractorResult(self._nlu_source_items[ind], sample_features))
        return result

    def _convert_items(self, classifier):
        # type: (AnyStr) -> List[Tuple[int, AnyStr, SampleFeatures]]

        result = []
        for ind, (classifiers, intent) in enumerate(izip(self._classifier_lists, self._intents)):
            if classifier not in classifiers:
                continue
            result.append((ind, intent, self._convert_item(ind)))

        return result

    def _convert_item(self, ind):
        dense = {
            feat_type: feat_matrix[ind] for feat_type, feat_matrix in self._dense_embeddings.iteritems()
        }

        seq_begin_pos, seq_end_pos = self._sample_ranges[ind]

        dense_seq, dense_seq_ids, sparse_seq = {}, {}, {}
        for feat_type, feat_matrix in self._dense_seq_embeddings.iteritems():
            if self._dense_seq_source_types[feat_type] == DenseSeqSourceType.DENSE_SEQ:
                dense_seq[feat_type] = feat_matrix[seq_begin_pos: seq_end_pos]
            elif self._dense_seq_source_types[feat_type] == DenseSeqSourceType.DENSE_SEQ_IDS:
                dense_seq_ids[feat_type] = list(feat_matrix[seq_begin_pos: seq_end_pos, 0])
            elif self._dense_seq_source_types[feat_type] == DenseSeqSourceType.SPARSE_SEQ:
                feature_list = self._sparse_feature_lists[feat_type]
                feat_matrix = feat_matrix[seq_begin_pos: seq_end_pos]
                sparse_seq[feat_type] = [[SparseFeatureValue(feature_list[feat_val[0]])] for feat_val in feat_matrix]

        for feat_type, feat_matrix in self._sparse_embeddings.iteritems():
            feature_list = self._sparse_feature_lists[feat_type]
            feat_matrix = feat_matrix[seq_begin_pos: seq_end_pos].tocoo()
            sparse_seq[feat_type] = [[] for _ in xrange(seq_end_pos - seq_begin_pos)]
            for coo_row, coo_col, coo_data in zip(feat_matrix.row, feat_matrix.col, feat_matrix.data):
                feat = SparseFeatureValue(feature_list[coo_col], coo_data)
                sparse_seq[feat_type][coo_row].append(feat)

        sample = Sample.from_string(self._preprocessed_texts[ind], tags=self._sample_tags[ind])
        sample._weight = self._weights[ind]

        return SampleFeatures(sample=sample, dense=dense, dense_seq=dense_seq,
                              dense_seq_ids=dense_seq_ids, sparse_seq=sparse_seq)

    def to_dataset_view(self, classifier, features=None, embeddings_matrices=None, use_intents=None):
        # type: (AnyStr, Container[AnyStr], Optional[Mapping[AnyStr, Tuple[AnyStr, np.ndarray]]]) -> VinsDatasetView

        """Converts object to VinsDatasetView with items relevant to the classifier.
        :param classifier: name of classifier by which items are filtered.
            Can be None - in this case all items are converted.
        :param features: list of features' names which should appear in the result VinsDatasetView
        :param embeddings_matrices: mapping from dense_seq_ids features' names to pair:
            <new feature name, corresponding embeddings matrix>.
        :return: VinsDatasetView
        """

        intents, item_indices = [], []
        for index, intent in enumerate(self._iter_intents(holdout_classifiers=[classifier])):
            if intent and (not use_intents or intent in use_intents):
                intents.append(intent)
                item_indices.append(index)

        sample_ranges = [self._sample_ranges[ind] for ind in item_indices]
        assert len(sample_ranges) == len(item_indices)

        if features is None or 'texts' in features:
            preprocessed_texts = [self._preprocessed_texts[ind] for ind in item_indices]
            assert len(sample_ranges) == len(preprocessed_texts)
        else:
            preprocessed_texts = None

        dense_embeddings = {
            feat_type: feat_matrix[item_indices] for feat_type, feat_matrix in self._dense_embeddings.iteritems()
        }

        dense_seq_embeddings = self._filter_mapping(self._dense_seq_embeddings, features)
        if embeddings_matrices is not None:
            dense_seq_embeddings.update({
                embeddings_matrices[feat_type][0]: embeddings_matrices[feat_type][1][feat_matrix.squeeze(-1)]
                for feat_type, feat_matrix in self._dense_seq_embeddings.iteritems()
                if feat_type in embeddings_matrices and embeddings_matrices[feat_type][0] in features
            })

        additional_infos = [self._additional_infos[ind] for ind in item_indices]

        classifier_feature_mappings = self._filter_mapping(self._classifier_feature_mappings, features)
        return VinsDatasetView(dataset_indices=item_indices,
                               sample_ranges=sample_ranges,
                               intents=intents,
                               preprocessed_texts=preprocessed_texts,
                               dense_embeddings=self._filter_mapping(dense_embeddings, features),
                               dense_seq_embeddings=dense_seq_embeddings,
                               sparse_embeddings=self._filter_mapping(self._sparse_embeddings, features),
                               features_indices=self._sparse_feature_mappings,
                               classifier_feature_mappings=classifier_feature_mappings,
                               additional_infos=additional_infos)

    def _iter_intents(self, holdout_classifiers=(), filtered_intents=None):
        # type: (Container[AnyStr], Optional[Container[AnyStr]])->Generator[List[AnyStr], None, None]

        filtered_intents = filtered_intents or ()
        for classifiers, intent in izip(self._classifier_lists, self._intents):
            if intent not in filtered_intents and any(classifier in holdout_classifiers for classifier in classifiers):
                yield intent
            else:
                yield None

    @staticmethod
    def _filter_mapping(embeddings, features):
        if features is None:
            return embeddings
        return {feat_type: feat_matrix for feat_type, feat_matrix in embeddings.iteritems() if feat_type in features}

    def add_feature(self, feature_name, feature_type, feature_matrix, feature_mapping):
        # type: (AnyStr, VinsDataset.FeatureType, Union[np.ndarray, sps.csr_matrix], Mapping[AnyStr, int]) -> NoReturn
        """Adds new feature to the dataset. The feature_matrix elements should be parallel to the dataset's items."""

        assert isinstance(feature_matrix, (np.ndarray, sps.csr_matrix)), 'Unknown feature matrix type'
        assert feature_matrix.shape[0] == len(self._sample_ranges), \
            'Invalid features count. Expected - {}, found - {}'.format(len(self._sample_ranges),
                                                                       feature_matrix.shape[0])
        if feature_type == self.FeatureType.DENSE:
            self._dense_embeddings[feature_name] = feature_matrix
        elif feature_type == self.FeatureType.DENSE_SEQ:
            self._dense_seq_embeddings[feature_name] = feature_matrix
            self._dense_seq_source_types[feature_name] = DenseSeqSourceType.DENSE_SEQ
        elif feature_type == self.FeatureType.SPARSE_SEQ:
            self._sparse_embeddings[feature_name] = feature_matrix
        elif feature_type == self.FeatureType.INDEX_SEQ:
            self._dense_seq_embeddings[feature_name] = feature_matrix
            self._dense_seq_source_types[feature_name] = DenseSeqSourceType.DENSE_SEQ_IDS

        self._classifier_feature_mappings[feature_name] = feature_mapping

    @classmethod
    def split(cls, dataset, holdout_classifiers=(), holdout_size=0.3, random_state=0):
        # type: (VinsDataset, Iterable[AnyStr], Union[float, int], int) -> Tuple[VinsDataset, VinsDataset]

        """Stratify split of the dataset into two parts. The holdout_size sets the size of the second dataset.
        The split is performed on the collection of (item, intent) pairs.
        The intents for these pairs are collected for each item from all its classifiers from holdout_classifiers.
        For instance, let the dataset have a single item with following classifier_to_intents mapping:
            {classifier1: [intent1, intent2], classifier2: [intent1, intent3]}
        and the classifier1 is in holdout_classifiers.
        Then the splittable part of dataset is [(item, intent1), (item, intent2)].
        As a result, if (item, intent1) fell into holdout split, the train_dataset would be
            [item, {classifier1: [intent2], classifier2: [intent3]}]
        and the holdout_dataset would be:
            [item, {classifier1: [intent1], classifier2: [intent1]}]
        Note that the holdout_classifiers param means only that (item, intent3) cannot appear in the holdout.
        """

        intents_counts = Counter(
            (intent for intent in dataset._iter_intents(holdout_classifiers=holdout_classifiers) if intent is not None)
        )
        single_element_intents = {intent for intent, count in intents_counts.iteritems() if count == 1}

        splittable_intents, dataset_indices = dataset._get_intents(holdout_classifiers=holdout_classifiers,
                                                                   filtered_intents=single_element_intents)

        splitter = StratifiedShuffleSplit(test_size=holdout_size, random_state=random_state)
        _, holdout_indices = splitter.split(np.zeros(len(splittable_intents)), splittable_intents).next()

        holdout_mask = np.zeros(len(dataset._intents), dtype=np.bool)
        holdout_mask[dataset_indices[holdout_indices]] = True

        return (cls._build_dataset_slice(dataset, ~holdout_mask),
                cls._build_dataset_slice(dataset, holdout_mask))

    def _get_intents(self, holdout_classifiers, filtered_intents):
        # type: (Iterable[AnyStr], Optional[Container[AnyStr]]) -> Tuple[List[AnyStr], List[int]]

        splittable_intents, dataset_indices = [], []
        for dataset_index, intent in enumerate(self._iter_intents(holdout_classifiers=holdout_classifiers,
                                                                  filtered_intents=filtered_intents)):
            if intent:
                splittable_intents.append(intent)
                dataset_indices.append(dataset_index)
        return splittable_intents, np.array(dataset_indices)

    @staticmethod
    def _build_dataset_slice(dataset, indices_mask):
        # type: (VinsDataset, np.array) -> VinsDataset

        def _get_embeddings_slice(embeddings, indices):
            return {feat_type: feat_matrix[indices] for feat_type, feat_matrix in embeddings.iteritems()}

        def _get_sample_ranges_slice(sample_ranges, indices):
            sample_ranges_slice = []
            cur_pos = 0
            for i in indices:
                sample_ranges_slice.append((cur_pos, cur_pos + sample_ranges[i][1] - sample_ranges[i][0]))
                cur_pos = sample_ranges_slice[-1][1]
            return sample_ranges_slice

        indices = indices_mask.nonzero()[0]

        sample_ranges = _get_sample_ranges_slice(dataset._sample_ranges, indices)

        dense_embeddings = _get_embeddings_slice(dataset._dense_embeddings, indices)

        seq_indices = [
            i for sample_ind in indices for i in xrange(dataset._sample_ranges[sample_ind][0],
                                                        dataset._sample_ranges[sample_ind][1])
        ]
        dense_seq_embeddings = _get_embeddings_slice(dataset._dense_seq_embeddings, seq_indices)
        sparse_embeddings = _get_embeddings_slice(dataset._sparse_embeddings, seq_indices)

        preprocessed_texts = [dataset._preprocessed_texts[i] for i in indices]
        classifier_lists = [dataset._classifier_lists[i] for i in indices]
        intents = [dataset._intents[i] for i in indices]
        sample_tags = [dataset._sample_tags[i] for i in indices] if dataset._sample_tags else None
        nlu_source_items = [dataset._nlu_source_items[i] for i in indices]
        weights = dataset._weights[indices]
        additional_infos = [dataset._additional_infos[i] for i in indices]

        return VinsDataset(
            sample_ranges=sample_ranges,
            dense_embeddings=dense_embeddings,
            dense_seq_embeddings=dense_seq_embeddings,
            sparse_embeddings=sparse_embeddings,
            nlu_source_items=nlu_source_items,
            preprocessed_texts=preprocessed_texts,
            sample_tags=sample_tags,
            weights=weights,
            classifier_lists=classifier_lists,
            intents=intents,
            sparse_feature_mappings=dataset._sparse_feature_mappings,
            dense_seq_source_types=dataset._dense_seq_source_types,
            classifier_feature_mappings=dataset._classifier_feature_mappings,
            additional_infos=additional_infos
        )

    def save(self, path):
        # type: (AnyStr) -> NoReturn

        with TarArchive(path, mode='w') as archive:
            tmp_dir = archive.get_tmp_dir()

            if self._dense_embeddings:
                np.savez(os.path.join(tmp_dir, 'dense_embeddings.npz'), **self._dense_embeddings)
            if self._dense_seq_embeddings:
                np.savez(os.path.join(tmp_dir, 'dense_seq_embeddings.npz'), **self._dense_seq_embeddings)
            np.save(os.path.join(tmp_dir, 'weights.npy'), self._weights)

            for emb_name, emb_data in self._sparse_embeddings.iteritems():
                sps.save_npz(os.path.join(tmp_dir, emb_name + '_sparse_embeddings.npz'), emb_data)

            with open(os.path.join(tmp_dir, 'metainfo.pkl'), 'wb') as f:
                pickle.dump(
                    (self._sample_ranges, self._nlu_source_items, self._sparse_feature_mappings,
                     self._preprocessed_texts, self._sample_tags, self._classifier_lists, self._intents,
                     self._dense_seq_source_types, self._classifier_feature_mappings, self._additional_infos),
                    file=f, protocol=pickle.HIGHEST_PROTOCOL
                )

            archive.add('data', tmp_dir)

    @classmethod
    def restore(cls, path):
        # type: (AnyStr) -> VinsDataset

        def _load_npz(dir_path, file_path):
            file_path = os.path.join(dir_path, file_path)
            if os.path.isfile(file_path):
                embeddings = np.load(file_path)
                return {file_name: embeddings[file_name] for file_name in embeddings.files}
            return {}

        with TarArchive(path) as archive:
            tmp_dir = archive.get_tmp_dir()
            archive.extract_all(tmp_dir)

            tmp_dir = os.path.join(tmp_dir, 'data')

            dense_embeddings = _load_npz(tmp_dir, 'dense_embeddings.npz')
            dense_seq_embeddings = _load_npz(tmp_dir, 'dense_seq_embeddings.npz')
            weights = np.load(os.path.join(tmp_dir, 'weights.npy'))

            sparse_embeddings = {}
            for file_name in os.listdir(tmp_dir):
                if file_name.endswith('_sparse_embeddings.npz'):
                    data_name = file_name[:-len('_sparse_embeddings.npz')]
                    sparse_embeddings[data_name] = sps.load_npz(os.path.join(tmp_dir, file_name))

            with open(os.path.join(tmp_dir, 'metainfo.pkl'), 'rb') as f:
                (sample_ranges, nlu_source_items, sparse_feature_mappings, preprocessed_texts,
                 sample_tags, classifier_lists, intents, dense_seq_source_types,
                 classifier_feature_mappings, additional_info) = pickle.load(f)

        return cls(sample_ranges=sample_ranges,
                   sparse_feature_mappings=sparse_feature_mappings,
                   dense_embeddings=dense_embeddings,
                   dense_seq_embeddings=dense_seq_embeddings,
                   sparse_embeddings=sparse_embeddings,
                   nlu_source_items=nlu_source_items,
                   preprocessed_texts=preprocessed_texts,
                   sample_tags=sample_tags,
                   dense_seq_source_types=dense_seq_source_types,
                   weights=weights,
                   classifier_lists=classifier_lists,
                   intents=intents,
                   classifier_feature_mappings=classifier_feature_mappings,
                   additional_infos=additional_info)


class VinsDatasetBuilder(object):
    """Converts features collected with FeatureExtractorFromItem to VinsDataset.
    Can build new dataset or merge the features to an existing one.
    """

    def __init__(self, intent_to_results=None, sample_infos=None):
        assert (intent_to_results is not None) ^ (sample_infos is not None)

        self._intent_to_results = intent_to_results
        self._sample_infos = sample_infos
        self._length = None
        self._seq_length = None
        self._dense_embeddings = None
        self._dense_seq_embeddings = None
        self._sparse_feature_mappings = None
        self._sparse_embeddings = None
        self._dense_seq_source_types = None
        self._sample_ranges = None
        self._nlu_source_items = None
        self._preprocessed_texts = None
        self._sample_tags = None
        self._weights = None
        self._classifier_lists = None
        self._intents = None

    def _build(self):
        self._length = sum(1 for _, result, _ in self._iter_data())
        self._seq_length = sum(len(result.sample_features) for _, result, _ in self._iter_data())

        (dense_feature_types, dense_seq_feature_types,
         sparse_seq_feature_types, dense_seq_ids_feature_types) = self._collect_feature_types()
        self._init_feature_source_types(dense_seq_feature_types, sparse_seq_feature_types, dense_seq_ids_feature_types)
        self._build_feature_matrices(dense_feature_types, dense_seq_feature_types,
                                     sparse_seq_feature_types, dense_seq_ids_feature_types)

        (self._sample_ranges, self._nlu_source_items, self._preprocessed_texts, self._sample_tags,
         self._weights, self._classifier_lists, self._intents, self._additional_infos) = self._build_metadata()

    def _iter_data(self):
        # type: () -> Iterator[AnyStr, FeatureExtractorResult, Any]

        if self._intent_to_results:
            for intent, results in self._intent_to_results.iteritems():
                for result in results:
                    if not isinstance(result, FeatureExtractorResult):
                        continue
                    yield intent, result, None
        else:
            for intent, result, additional_info in self._sample_infos:
                if not isinstance(result, FeatureExtractorResult):
                    continue
                yield intent, result, additional_info

    @staticmethod
    def _to_item_without_classifiers(item):
        # type: (NluSourceItem) -> NluSourceItem

        return NluSourceItem(text=item.text, original_text=item.original_text, slots=item.slots,
                             can_use_to_train_tagger=item.can_use_to_train_tagger)

    def _collect_feature_types(self):
        # type: () -> Tuple[Set[AnyStr], Set[AnyStr], Dict[AnyStr, bool], Set[AnyStr]]

        dense_feature_types = set()
        dense_seq_feature_types = set()
        sparse_seq_feature_types = defaultdict(lambda: True)
        dense_seq_ids_feature_types = set()

        for _, res, _ in self._iter_data():
            dense_feature_types.update(res.sample_features.dense.keys())
            dense_seq_feature_types.update(res.sample_features.dense_seq.keys())
            dense_seq_ids_feature_types.update(res.sample_features.dense_seq_ids.keys())

            for feat_type in res.sample_features.sparse_seq:
                for feat_seq in res.sample_features.sparse_seq[feat_type]:
                    sparse_seq_feature_types[feat_type] = sparse_seq_feature_types[feat_type] and len(feat_seq) == 1

        self._log_feature_types(dense_feature_types, dense_seq_feature_types,
                                sparse_seq_feature_types, dense_seq_ids_feature_types)

        return dense_feature_types, dense_seq_feature_types, sparse_seq_feature_types, dense_seq_ids_feature_types

    def _log_feature_types(self, dense_feature_types, dense_seq_feature_types,
                           sparse_seq_feature_types, dense_seq_ids_feature_types):
        logger.info('Loaded following features:')
        logger.info('Dense features: %s', ' '.join(dense_feature_types))
        logger.info('Dense_seq features: %s', ' '.join(dense_seq_feature_types))
        logger.info('Sparse features: %s', ' '.join(
            feat_type for feat_type, is_index in sparse_seq_feature_types.iteritems() if not is_index))
        logger.info('Sparse index features: %s %s',
                    ' '.join(feat_type for feat_type, is_index in sparse_seq_feature_types.iteritems() if is_index),
                    ' '.join(feat_type for feat_type in dense_seq_ids_feature_types))

    def _init_feature_source_types(self, dense_seq_feature_types, sparse_seq_feature_types,
                                   dense_seq_ids_feature_types):
        self._dense_seq_source_types = {}
        for feat_type in dense_seq_feature_types:
            self._dense_seq_source_types[feat_type] = DenseSeqSourceType.DENSE_SEQ
        for feat_type in dense_seq_ids_feature_types:
            self._dense_seq_source_types[feat_type] = DenseSeqSourceType.DENSE_SEQ_IDS
        for feat_type, is_index in sparse_seq_feature_types.iteritems():
            if is_index:
                self._dense_seq_source_types[feat_type] = DenseSeqSourceType.SPARSE_SEQ

    def _collect_sparse_features_indices(self, features_indices=None):
        # type: (Dict[AnyStr, Dict[AnyStr, int]]) -> Dict[AnyStr, Dict[AnyStr, int]]

        features_indices = features_indices or defaultdict(dict)
        for _, res, _ in self._iter_data():
            for feat_type in res.sample_features.sparse_seq:
                for feat_seq in res.sample_features.sparse_seq[feat_type]:
                    for feat in feat_seq:
                        if feat.value not in features_indices[feat_type]:
                            features_indices[feat_type][feat.value] = len(features_indices[feat_type])
        return features_indices

    def _build_feature_matrices(self, dense_feature_types, dense_seq_feature_types,
                                sparse_seq_feature_types, dense_seq_ids_feature_types):
        self._dense_embeddings = {
            feat_type: self._build_dense_embeddings(feat_type, attrgetter('dense'), is_seq_feature=False)
            for feat_type in dense_feature_types
        }

        self._dense_seq_embeddings = {
            feat_type: self._build_dense_embeddings(feat_type, attrgetter('dense_seq'), is_seq_feature=True)
            for feat_type in dense_seq_feature_types
        }

        self._sparse_embeddings = {
            feat_type: self._build_sparse_embeddings(feat_type, self._seq_length, attrgetter('sparse_seq'))
            for feat_type, is_index in sparse_seq_feature_types.iteritems() if not is_index
        }

        self._dense_seq_embeddings.update({
            feat_type: self._build_indices_embeddings(feat_type, features_count=self._seq_length,
                                                      feat_attr_getter=attrgetter('sparse_seq'))
            for feat_type, is_index in sparse_seq_feature_types.iteritems() if is_index
        })

        self._dense_seq_embeddings.update({
            feat_type: self._build_indices_embeddings(feat_type, features_count=self._seq_length,
                                                      feat_attr_getter=attrgetter('dense_seq_ids'),
                                                      item_index_getter=lambda ind: ind)
            for feat_type in dense_seq_ids_feature_types
        })

    def _build_dense_embeddings(self, feat_type, feat_attr_getter, is_seq_feature):
        # type: (AnyStr, Callable, bool) -> np.ndarray

        feat_dim, feat_dtype = None, None
        for _, res, _ in self._iter_data():
            features = feat_attr_getter(res.sample_features)
            if feat_type in features:
                feat_dim = features[feat_type].shape[-1]
                feat_dtype = features[feat_type].dtype
                break

        assert feat_dim is not None and feat_dtype is not None, \
            'Dense feature {} not found in samples'.format(feat_type)

        features_count = self._seq_length if is_seq_feature else self._length
        embeddings = np.zeros((features_count, feat_dim), dtype=feat_dtype)

        pos = 0
        for _, res, _ in self._iter_data():
            features = feat_attr_getter(res.sample_features)
            sample_len = len(res.sample_features) if is_seq_feature else 1
            if feat_type in features:
                embeddings[pos: pos + sample_len] = features[feat_type]
            pos += sample_len
        return embeddings

    def _build_sparse_embeddings(self, feat_type, features_count, feat_attr_getter):
        # type: (AnyStr, int, Callable) -> sps.csr_matrix

        feature_index = self._sparse_feature_mappings[feat_type]

        rows, cols, vals = [], [], []
        row = 0
        for _, res, _ in self._iter_data():
            features = feat_attr_getter(res.sample_features)
            if feat_type not in features:
                row += len(res.sample_features)
                continue

            for variants in features[feat_type]:
                rows.extend([row] * len(variants))
                cols.extend((feature_index[var.value] for var in variants))
                vals.extend((var.weight for var in variants))
                row += 1

        rows, cols, vals = np.array(rows), np.array(cols), np.array(vals)
        embeddings = sps.coo_matrix((vals, (rows, cols)), shape=(features_count, len(feature_index)), dtype=np.float64)
        return embeddings.tocsr()

    def _build_indices_embeddings(self, feat_type, features_count, feat_attr_getter, item_index_getter=None):
        # type: (AnyStr, int, Callable, Callable) -> np.ndarray

        def sparse_seq_feature_getter(item_seq):
            return self._sparse_feature_mappings[feat_type][item_seq[0].value]

        item_index_getter = item_index_getter or sparse_seq_feature_getter

        embeddings = np.zeros(shape=(features_count, 1), dtype=np.int32)
        pos = 0
        for _, res, _ in self._iter_data():
            features = feat_attr_getter(res.sample_features)
            if feat_type in features:
                embeddings[pos: pos + len(res.sample_features), 0] = [
                    item_index_getter(item_seq) for item_seq in features[feat_type]
                ]

            pos += len(res.sample_features)

        return embeddings

    def _build_metadata(self):
        # type: () -> Tuple[List, List, List, List, np.ndarray, List, List, List]

        sample_ranges, nlu_source_items, preprocessed_texts, sample_tags = [], [], [], []
        weights, classifier_lists, intents, additional_infos = [], [], [], []
        cur_pos = 0
        for intent, res, additional_info in self._iter_data():
            sample_ranges.append((cur_pos, cur_pos + len(res.sample_features)))
            cur_pos += len(res.sample_features)

            nlu_source_items.append(res.item)
            preprocessed_texts.append(res.sample_features.sample.text)
            sample_tags.append(res.sample_features.sample.tags)
            weights.append(res.sample_features.sample.weight)

            trainable_classifiers = list(res.item.trainable_classifiers)
            if res.item.can_use_to_train_tagger:
                trainable_classifiers.append('taggers')
            classifier_lists.append(trainable_classifiers)

            intents.append(intent)
            additional_infos.append(additional_info)

        return (sample_ranges, nlu_source_items, preprocessed_texts, sample_tags,
                np.array(weights), classifier_lists, intents, additional_infos)

    def build(self):
        # type: () -> VinsDataset
        """Builds the dataset"""

        self._sparse_feature_mappings = self._collect_sparse_features_indices()
        self._build()

        return VinsDataset(sample_ranges=self._sample_ranges,
                           dense_embeddings=self._dense_embeddings,
                           dense_seq_embeddings=self._dense_seq_embeddings,
                           sparse_embeddings=self._sparse_embeddings,
                           nlu_source_items=self._nlu_source_items,
                           preprocessed_texts=self._preprocessed_texts,
                           sample_tags=self._sample_tags,
                           weights=self._weights,
                           dense_seq_source_types=self._dense_seq_source_types,
                           classifier_lists=self._classifier_lists,
                           intents=self._intents,
                           sparse_feature_mappings=self._sparse_feature_mappings,
                           classifier_feature_mappings={},
                           additional_infos=self._additional_infos)

    def merge_to(self, initial_dataset):
        # type: (VinsDataset) -> VinsDataset
        """Merges dataset to an existing one. Returns merged dataset.
        Warning: changes initial_dataset.
        """
        self._sparse_feature_mappings = self._collect_sparse_features_indices(initial_dataset._sparse_feature_mappings)
        self._build()

        _, dense_seq_features, sparse_seq_feature, dense_seq_ids_features = self._collect_feature_types()

        for feat_type in dense_seq_features:
            if feat_type not in initial_dataset._dense_seq_source_types:
                initial_dataset._dense_seq_source_types[feat_type] = DenseSeqSourceType.DENSE_SEQ

        for feat_type in dense_seq_ids_features:
            if feat_type not in initial_dataset._dense_seq_source_types:
                initial_dataset._dense_seq_source_types[feat_type] = DenseSeqSourceType.DENSE_SEQ_IDS

        for feat_type, is_index in sparse_seq_feature.iteritems():
            if is_index and feat_type not in initial_dataset._dense_seq_source_types:
                initial_dataset._dense_seq_source_types[feat_type] = DenseSeqSourceType.SPARSE_SEQ

        initial_dataset_seq_len = initial_dataset._sample_ranges[-1][1]
        self._sample_ranges = [
            (sample_range[0] + initial_dataset_seq_len, sample_range[1] + initial_dataset_seq_len)
            for sample_range in self._sample_ranges
        ]

        initial_dataset._sample_ranges.extend(self._sample_ranges)
        initial_dataset._nlu_source_items.extend(self._nlu_source_items)
        initial_dataset._preprocessed_texts.extend(self._preprocessed_texts)
        initial_dataset._sample_tags.extend(self._sample_tags)
        initial_dataset._classifier_lists.extend(self._classifier_lists)
        initial_dataset._intents.extend(self._intents)
        initial_dataset._weights = np.concatenate((initial_dataset._weights, self._weights), axis=0)
        initial_dataset._additional_infos.extend(self._additional_infos)

        initial_dataset._sparse_feature_mappings = self._sparse_feature_mappings
        initial_dataset.build_feature_list()

        self._merge_dense_embeddings(initial_dataset._dense_embeddings, len(initial_dataset._nlu_source_items),
                                     self._dense_embeddings, self._length)
        self._merge_dense_embeddings(initial_dataset._dense_seq_embeddings, initial_dataset_seq_len,
                                     self._dense_seq_embeddings, self._seq_length)
        self._merge_sparse_embeddings(initial_dataset, initial_dataset_seq_len)

        return initial_dataset

    @staticmethod
    def _merge_dense_embeddings(initial_embeddings, initial_len, new_embeddings, new_len):
        for feat_type, feat_matrix in initial_embeddings.iteritems():
            if feat_type in new_embeddings:
                new_feat_matrix = np.concatenate((feat_matrix, new_embeddings[feat_type]), axis=0)
            else:
                new_feat_matrix = np.concatenate((feat_matrix, np.zeros((new_len, feat_matrix.shape[-1]))), axis=0)
            initial_embeddings[feat_type] = new_feat_matrix

        for feat_type, feat_matrix in new_embeddings.iteritems():
            if feat_type not in initial_embeddings:
                new_feat_matrix = np.concatenate((np.zeros(initial_len, feat_matrix.shape[-1]), feat_matrix), axis=0)
                initial_embeddings[feat_type] = new_feat_matrix

    def _merge_sparse_embeddings(self, initial_dataset, initial_dataset_seq_len):
        for feat_type, feat_matrix in initial_dataset._sparse_embeddings.iteritems():
            if feat_type in self._sparse_embeddings:
                if feat_matrix.shape[1] < self._sparse_embeddings[feat_type].shape[1]:
                    csr_matrix_init_data = (feat_matrix.data, feat_matrix.indices, feat_matrix.indptr)
                    csr_matrix_init_shape = (feat_matrix.shape[0], self._sparse_embeddings[feat_type].shape[1])
                    feat_matrix = sps.csr_matrix(csr_matrix_init_data, copy=False, shape=csr_matrix_init_shape)

                assert feat_matrix.shape[1] == self._sparse_embeddings[feat_type].shape[1]

                new_feat_matrix = sps.vstack((feat_matrix, self._sparse_embeddings[feat_type]), format='csr')
                initial_dataset._sparse_embeddings[feat_type] = new_feat_matrix
            else:
                csr_matrix_init_data = (feat_matrix.data, feat_matrix.indices, feat_matrix.indptr)
                csr_matrix_init_shape = (feat_matrix.shape[0] + self._seq_length, feat_matrix.shape[1])
                initial_dataset._sparse_embeddings[feat_type] = sps.csr_matrix(csr_matrix_init_data, copy=False,
                                                                               shape=csr_matrix_init_shape)

        for feat_type, feat_matrix in self._sparse_embeddings.iteritems():
            if feat_type not in initial_dataset._sparse_embeddings:
                empty_matrix = sps.csr_matrix(shape=(initial_dataset_seq_len, feat_matrix.shape[1]))
                initial_dataset._sparse_embeddings[feat_type] = sps.vstack((empty_matrix, feat_matrix), format='csr')
