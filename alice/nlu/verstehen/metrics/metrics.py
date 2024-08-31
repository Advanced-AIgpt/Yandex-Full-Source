import numpy as np
from sklearn.metrics.pairwise import euclidean_distances


def recall_at_k(query, neighbours, k, reduce=True):
    """
    Calculating Recall@K metric values for retrieval. Which is, for the `k` value, an
    average portion times where at least one of first `k` `neighbours` is the same as
    `query`. The same as https://www.tensorflow.org/api_docs/python/tf/metrics/recall_at_k

    Arguments:
        query: array of shape [n_samples,]
        neighbours: array of shape [n_samples, N], where N >= max(k)
        k: k values to compute which are used in Recall@K metric (can be int or list)
        reduce: bool value of whether to reduce recalls for k value to a mean

    Returns:
        list of Recall@K for each value in K
    """
    if isinstance(k, int):
        k = [k]

    if max(k) > neighbours.shape[1]:
        raise ValueError(
            'Recall@K is computed over max(k) equal to {} but number of neighbours '
            'specified is {}. Number of neighbours should be at least equal to max(k)'.format(
                max(k), neighbours.shape[1]
            )
        )

    query = np.expand_dims(query, axis=1)

    recalls = []
    for cur_k in k:
        recall_at_i = np.any(query == neighbours[:, :cur_k], axis=1)
        if reduce:
            recall_at_i = np.mean(recall_at_i)
        recalls.append(recall_at_i)
    return recalls


def precision_at_k(query, neighbours, k, reduce=True):
    """
    Calculating Precision@K metric values for retrieval. Which is, for the `k` value, an
    average portion of first `k` `neighbours` being the same as `query`. The same as
    https://www.tensorflow.org/api_docs/python/tf/metrics/precision_at_k

    Arguments:
        query: array of shape [n_samples,]
        neighbours: array of shape [n_samples, N], where N >= max(k)
        k: k values to compute which are used in Precision@K metric (can be int or list)
        reduce: bool value of whether to reduce precisions for k value to a mean

    Returns:
        list of Precision@K for each value in K
    """
    if isinstance(k, int):
        k = [k]

    if max(k) > neighbours.shape[1]:
        raise ValueError(
            'Precision@K is computed over max(k) equal to {} but number of neighbours '
            'specified is {}. Number of neighbours should be at least equal to max(k)'.format(
                max(k), neighbours.shape[1]
            )
        )

    query = np.expand_dims(query, axis=1)

    precisions = []
    for cur_k in k:
        precisions_at_i = np.mean(query == neighbours[:, :cur_k], axis=1)
        if reduce:
            precisions_at_i = np.mean(precisions_at_i)
        precisions.append(precisions_at_i)
    return precisions


def r_precision(query, neighbours, r_map, max_r=None, reduce_method='avg'):
    """
    Calculating R-Precision metric value for retrieval. Which is the average portion of
    first `r` `neighbours` being the same as `query` where `r` is the number of relevant
    elements that can be in neighbours and can vary for different `query` but not higher
    than `max_r` (if specified). It is equivalent to Precision@K where K is is different
    for different `query` item and is equal to the number of relevant items that can be
    retrieved from neighbours.
    See: https://en.wikipedia.org/wiki/Evaluation_measures_(information_retrieval)

    Arguments:
        query: array of shape [n_samples,]
        neighbours: array of shape [n_samples, r] or list of n_samples arrays of shape [r]
        r_map: dict where keys are `query` items and values are `r` numbers of relevant values
            for a particular `query` value.
        max_r: the max number of relevant items that any `query` item can have in `neighbours`.
            If None, then there is no limitation and `relevant_items_map` values are used.
        reduce_method: str value of how to reduce all precision values to a single scalar.
            Possible values are: 'avg' for mean, 'weighted_avg' for mean weighted by r,
            'sqrt_weighted_avg' for mean weighted by square roots of r, None for no reducing.

    Returns:
        value of R-Precision
    """

    # TODO add check for sizes of neighbours?
    precisions, r_values = [], []
    for current_query, current_neighbours in zip(query, neighbours):
        r = min(r_map[current_query], len(current_neighbours))

        if max_r is not None:
            r = min(max_r, r)
        r_values.append(r)

        # there is no items could be found in current neighbours anyway
        if r == 0:
            precisions.append(0.0)
        else:
            precisions.append(np.sum(current_query == current_neighbours[:r]) / float(r))

    if reduce_method == 'avg':
        precisions = np.mean(precisions)
    elif reduce_method in ['weighted_avg', 'sqrt_weighted_avg']:
        r_values = np.array(r_values, dtype=np.float)
        if reduce_method == 'sqrt_weighted_avg':
            r_values = np.sqrt(r_values)
        weights = r_values / np.sum(r_values)
        precisions = np.sum(np.array(precisions) * weights)
    elif reduce_method is not None:
        raise ValueError('Reduce method {} is not recognized'.format(reduce_method))
    return precisions


def mean_pairwise_distance(points):
    """
    Returns mean pairwise distance for given points
    """
    distance_matrix = euclidean_distances(points)
    n = distance_matrix.shape[0]
    return np.sum(np.tril(distance_matrix)) / ((n ** 2 - n) / 2)
