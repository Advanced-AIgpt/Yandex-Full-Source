# -*- coding: utf-8 -*-
from __future__ import absolute_import
from __future__ import print_function
from __future__ import unicode_literals
from __future__ import division
import collections
import numpy as np
from itertools import chain
from copy import deepcopy
# from typing import List
# from typing import Optional

DEFAULT_PENALTY_FOR_UNKNOWN_WORDS = 0.25
DEFAULT_UNKNOWN_WORD_MARK = "<SPN>"

WTNEdge = collections.namedtuple('WTNEdge', 'value score sources original_positions')
TextHyp = collections.namedtuple('TextHyp', 'object_id source_id value')
"""
TextHyp.value can be one of
string, list of strings, WordTransitionNetwork
Tuple (object_id, source_id, [original_position]) can be used to store additional info for future aggregation
"""

AlignmentResult = collections.namedtuple('AlignmentResult', 'action hypothesis_word reference_word')


class WordTransitionNetwork:
    """
    Class representing word transition network structure for ROVER with additional info
    """
    def __init__(
            self,
            object_id,
            hypotheses,  # type: List[TextHyp]
            cluster_references=None,
            unknown_word_mark=DEFAULT_UNKNOWN_WORD_MARK  # type: Optional[unicode]
    ):
        """
        :param object_id: id of object associated with this WTN instance
        :param hypotheses: list of Hypo instances to build WTN
        :param cluster_references: cluster references to use
        :param unknown_word_mark: considered as a some set of unknown words, set None to not use this feature
        """
        self.object_id = object_id
        self.crs = cluster_references
        assert len(hypotheses) >= 1
        hyp = hypotheses[0]
        self.hypotheses_sources = [hyp.source_id]
        self.edges = None
        self.use_unknown_word_mark = unknown_word_mark is not None
        self.unknown_word_mark = unknown_word_mark
        self._build_one_hyp(hyp)
        if self.use_unknown_word_mark:
            assert len(hypotheses) == 1  # not sure how it works otherwise
        for hyp in hypotheses[1:]:
            self.merge_with(WordTransitionNetwork(object_id, [hyp], cluster_references))

    def _add_cluster_references(self, words, source_id):
        # iterate over all substrings and add edges from cluster references
        len_words = len(words)
        insertions_before = [0 for _ in range(len_words)]
        for i in range(len_words):
            for j in range(i, len_words):
                possible_crs = self.crs.get(tuple(words[i:(j + 1)]), None)
                # firstly we start from i
                if possible_crs is None:
                    continue
                for cluster_reference in possible_crs:
                    start_pos = sum(insertions_before[:i]) + i
                    end_pos = sum(insertions_before[:j+1]) + j
                    aligned_fragment, actions = self._align(
                        self.edges[start_pos:end_pos + 1],
                        [{word: WTNEdge(word, None, [source_id], [None])} for word in cluster_reference],
                        [source_id],
                        [source_id]
                    )
                    self.edges = self.edges[:start_pos] + aligned_fragment + self.edges[end_pos + 1:]
                    pos = i
                    for action in actions:
                        if action == "I" or action == "CI":
                            insertions_before[pos] += 1
                        else:
                            pos += 1
                    assert pos == j + 1

    def _build_one_hyp(self, hyp):
        assert hyp.object_id == self.object_id
        if isinstance(hyp.value, str):
            words = [w.lower() if w != self.unknown_word_mark and "<" not in w else w
                     for w in hyp.value.decode("utf8").strip().split()]
        else:
            words = hyp.value
        self.edges = [{word: WTNEdge(word, None, [hyp.source_id], [i])} for i, word in enumerate(words)]
        if self.crs:
            self._add_cluster_references(words, hyp.source_id)

    def __len__(self):
        return len(self.edges)

    def __getitem__(self, indx):
        return self.edges[indx]

    def __repr__(self):
        return "Word transition network with edges:\n" + ";\n".join(
            [str(self[i]) for i in range(len(self))]
        ) + "\n\n"

    def _align(self, ref, hyp, ref_sources, hyp_sources):
        reference_length = len(ref)
        hypo_length = len(hyp)
        distance = np.full((hypo_length + 1, reference_length + 1), np.inf, dtype=np.float)
        distance[0][0] = 0
        # distance[0] = np.arange(hypo_length + 1)
        # distance[:, 0] = np.arange(reference_length + 1)
        memoization = [[tuple([(-1, -1), AlignmentResult('A', set(), set())])
                        for _ in range(reference_length + 1)]
                       for _ in range(hypo_length + 1)]

        for i, hyp_edges in enumerate(chain([dict()], hyp)):
            hyp_words_set = set(hyp_edges.keys())
            for j, ref_edges in enumerate(chain([dict()], ref)):
                ref_words_set = set(ref_edges.keys())
                if i > 0 and j > 0 and distance[i][j] >= distance[i - 1][j - 1] and \
                        len(ref_words_set & hyp_words_set) != 0:
                    distance[i][j] = distance[i - 1][j - 1]
                    memoization[i][j] = ("C", ref_edges, hyp_edges)
                if self.use_unknown_word_mark and i > 0 and j > 0 and \
                        distance[i][j] >= distance[i - 1][j - 1] + 0.001 and \
                        self.unknown_word_mark in hyp_words_set:  # replace with self.unknown_word_mark is free
                    distance[i][j] = distance[i - 1][j - 1] + 0.001
                    memoization[i][j] = ("U", ref_edges, hyp_edges)
                if i > 0 and distance[i][j] > distance[i - 1][j] and "" in hyp_edges:
                    distance[i][j] = distance[i - 1][j]  # free insertion if "" in hyp
                    memoization[i][j] = ("IC", {"": WTNEdge("", None, ref_sources, [None for _ in ref_sources])},
                                         hyp_edges,
                                         )
                if j > 0 and distance[i][j] > distance[i][j - 1] and "" in ref_edges:
                    distance[i][j] = distance[i][j - 1]  # free deletion if "" in ref
                    memoization[i][j] = ("DC", ref_edges,
                                         {"": WTNEdge("", None, hyp_sources, [None for _ in hyp_sources])})
                if self.use_unknown_word_mark and j > 0 and distance[i][j] + 0.001 > distance[i][j - 1] and \
                        memoization[i][j - 1][0] in ("U", "DU"):
                    distance[i][j] = distance[i][j - 1] + 0.001  # free deletion if there was self.unknown
                    # _word_mark replace before but
                    # we add 0.001 to avoid preference of longer ways through WTN
                    memoization[i][j] = ("DU", ref_edges,
                                         {
                                             self.unknown_word_mark: WTNEdge(
                                                 self.unknown_word_mark, None, hyp_sources, [None for _ in hyp_sources]
                                             )
                                          }
                                         )
                if i > 0 and j > 0 and distance[i][j] > distance[i-1][j-1] + 1 and \
                        len(ref_words_set & hyp_words_set) == 0:
                    distance[i][j] = distance[i - 1][j - 1] + 1
                    memoization[i][j] = ("S", ref_edges, hyp_edges)
                if i > 0 and distance[i][j] > distance[i - 1][j] + 1:
                    distance[i][j] = distance[i - 1][j] + 1
                    if self.use_unknown_word_mark and self.unknown_word_mark in hyp_words_set:
                        memoization[i][j] = ("IU", {"": WTNEdge("", None, ref_sources, [None for _ in ref_sources])},
                                             hyp_edges,
                                             )
                    else:
                        memoization[i][j] = ("I", {"": WTNEdge("", None, ref_sources, [None for _ in ref_sources])},
                                             hyp_edges,
                                             )
                if j > 0 and distance[i][j] > distance[i][j - 1] + 1:
                    distance[i][j] = distance[i][j - 1] + 1
                    memoization[i][j] = ("D", ref_edges,
                                         {"": WTNEdge("", None, hyp_sources, [None for _ in hyp_sources])})

        actions = list()
        alignment = list()
        i = hypo_length
        j = reference_length
        while i != 0 or j != 0:
            action, ref_edges, hyp_edges = memoization[i][j]
            joined_edges = deepcopy(ref_edges)
            for word, edge in hyp_edges.items():
                if word not in joined_edges:
                    joined_edges[word] = edge
                else:
                    value, score, sources, original_positions = joined_edges[word]
                    joined_edges[word] = WTNEdge(
                        value, score, sources + edge.sources, original_positions + edge.original_positions
                    )
            alignment.append(joined_edges)
            actions.append(action)
            if action == "C" or action == "S" or action == "U":
                i -= 1
                j -= 1
            elif action == "I" or action == "IC" or action == "IU":
                i -= 1
            else:  # action == "D" or action == "DC" or action == "DU":
                j -= 1
        return alignment[::-1], actions[::-1]

    def merge_with(self, wtn):
        assert self.object_id == wtn.object_id
        self.edges, _ = self._align(self.edges, wtn.edges, self.hypotheses_sources, wtn.hypotheses_sources)
        self.hypotheses_sources += wtn.hypotheses_sources

    def edit_distance(self, wtn):
        _, actions = self._align(self.edges, wtn.edges, self.hypotheses_sources, wtn.hypotheses_sources)
        return sum(1 for action in actions if action in ['S', 'D', 'I'])

    def calculate_wer(self, wtn):
        """
        Calculates WER for self as reference and wtn as hypothesis.
        Note that cluster reference are context free
        Words aligned with self.unknown_word_mark are ignored if it is set
        :return: tuple(float, float, float), WER computed for hyp and ref with WTN, Errors, len of reference
        """
        _, actions = self._align(self.edges, wtn.edges, self.hypotheses_sources, wtn.hypotheses_sources)
        errors = sum(1 for action in actions if action in ['S', 'D', 'I', 'IU'])
        ref_len = sum(1 for action in actions if action in ['C', 'D', 'S'])
        return errors/ref_len if ref_len != 0 else None, errors, ref_len

    def calculate_word_recall(self, wtn, coef_for_hyps_with_unknown_words=DEFAULT_PENALTY_FOR_UNKNOWN_WORDS):
        """
        Calculates part of skipped words for self as reference and wtn as hypothesis.
        Note that cluster reference are context free
        :return: tuple(float, float, float), word recall computed for hyp and ref with WTN, presented, len of reference
        """
        _, actions = self._align(self.edges, wtn.edges, self.hypotheses_sources, wtn.hypotheses_sources)
        ref_len = sum(1 for action in actions if action in ['C', 'D', 'S', 'U', 'DU'])
        presented_len = sum(1 for action in actions if action not in ['D', 'U', 'DU', 'IU'])
        recall = min(presented_len / ref_len if ref_len != 0 else 1, 1)
        if 'U' in actions:
            recall *= coef_for_hyps_with_unknown_words
        return recall, presented_len, ref_len


def calculate_wer(ref, hyp, cluster_references=None, unknown_word_mark=DEFAULT_UNKNOWN_WORD_MARK):
    """
    :param ref: string or list
    :param hyp: string or list
    :param cluster_references: ClusterReference or None
    :param unknown_word_mark: if not None considered as some set of unknown words
    :return: tuple(float, float, float), WER computed for hyp and ref with WTN, Errors, len of reference
    """
    if hyp is None and unknown_word_mark is not None:
        return 0.0, 0.0, 0.0
    ref_wtn = WordTransitionNetwork("tmp", [TextHyp("tmp", "ref", ref)], cluster_references, unknown_word_mark)
    hyp_wtn = WordTransitionNetwork("tmp", [TextHyp("tmp", "hyp", hyp)], cluster_references, unknown_word_mark)
    return ref_wtn.calculate_wer(hyp_wtn)


def calculate_word_recall(ref, hyp,
                          coef_for_hyps_with_unknown_words=DEFAULT_PENALTY_FOR_UNKNOWN_WORDS,
                          cluster_references=None,
                          unknown_word_mark=DEFAULT_UNKNOWN_WORD_MARK):
    """
    :param ref: string or list
    :param hyp: string or list
    :param coef_for_hyps_with_unknown_words: float
    :param cluster_references: ClusterReference or None
    :param unknown_word_mark: if not None considered as some set of unknown words
    :return: tuple(float, float, float), word recall computed for hyp and ref with WTN, presented, len of referencee
    """
    if hyp is None:
        return 0.0, 0.0, float(len(ref.split()))
    ref_wtn = WordTransitionNetwork("tmp", [TextHyp("tmp", "ref", ref)], cluster_references, unknown_word_mark)
    hyp_wtn = WordTransitionNetwork("tmp", [TextHyp("tmp", "hyp", hyp)], cluster_references, unknown_word_mark)
    return ref_wtn.calculate_word_recall(hyp_wtn, coef_for_hyps_with_unknown_words)
