# distutils: language = c++

cimport cython

from cython.operator cimport dereference
from cython.operator cimport preincrement
from libcpp.algorithm cimport partial_sort
from libcpp.unordered_map cimport unordered_map
from libcpp.pair cimport pair
from libcpp.vector cimport vector


cdef extern from "<algorithm>" namespace "std":
    T min[T](T a, T b)


cdef inline bint cmp_by_score(const pair[float, int]& lhs, const pair[float, int]& rhs):
    return lhs.first > rhs.first


cdef class Helper:
    cdef unordered_map[int, vector[int]] _target_indices

    def __cinit__(self, const unordered_map[int, vector[int]] target_indices):
        self._target_indices = target_indices

    @cython.boundscheck(False)
    @cython.wraparound(False)
    @cython.cdivision(True)
    def get_average_scores(self, float[:, ::1] outputs, const float[:, ::1] scores, int[:, ::1] argmax_y,
                           const int num_inputs, const int num_neighbors):
        cdef int label
        cdef vector[int]* idx
        cdef int cur_num_neighbors
        cdef vector[pair[float, int]] scores_idx
        cdef float the_sum = 0
        cdef unordered_map[int, vector[int]].iterator it
        cdef size_t i, j
        for i in range(num_inputs):
            it = self._target_indices.begin()
            while it != self._target_indices.end():
                label = dereference(it).first
                idx = &dereference(it).second
                cur_num_neighbors = min[int](num_neighbors, dereference(idx).size())
                if cur_num_neighbors > 0:
                    scores_idx.resize(dereference(idx).size())
                    for j in range(dereference(idx).size()):
                        scores_idx[j].second = dereference(idx)[j]
                        scores_idx[j].first = scores[dereference(idx)[j], i]
                    partial_sort(scores_idx.begin(),
                                 scores_idx.begin() + cur_num_neighbors,
                                 scores_idx.end(),
                                 &cmp_by_score)
                    the_sum = 0
                    for j in range(cur_num_neighbors):
                        the_sum += scores_idx[j].first
                    argmax_y[i, label] = scores_idx[0].second
                    outputs[i, label] = the_sum / cur_num_neighbors
                preincrement(it)
