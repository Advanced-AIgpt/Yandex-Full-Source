import numpy as np


def top_k_sorted_ids(array, k):
    """
    Getting indices of the `array` corresponding to top-k sorted values of the `array`
    in descending order.

    Average complexity of the method is believed to be O(k * log(k) + n) where `n` is the
    length of the `array`.
    """

    # If k is greater or equal to the array size, then no benefit in using the function
    # and we can just sort the whole array
    if k >= array.shape[0]:
        return np.argsort(array)[::-1]

    # Partitioning the array so we don't have to sort it all (O(n) on average)
    partition_ids = np.argpartition(array, array.shape[0] - k)

    # Taking only those ids that are in top-k
    partition_ids = partition_ids[-k:]

    # Slicing the initial array to perform sorting
    slice = array[partition_ids]

    # Getting argsort of array's values in descending order (O(k * log(k)))
    ids = np.argsort(slice)[::-1]

    # returning the sliced ids of the partition in the previously sorted order
    return partition_ids[ids]


def nearest_to_const_k_sorted_ids(array, k, const):
    """
    Getting indices of the `array` corresponding to sorted nearest-k to `const` values of the `array`
    in ascending order.

    Average complexity of the method is believed to be O(k * log(k) + n) where `n` is the
    length of the `array`.
    """
    return top_k_sorted_ids(-np.abs(array - const), k)


def random_from_batch_ids(array, k, lower_bound, upper_bound):
    """
    Returns k random indices from batch of array, the values of which are in the interval [lower_bound; upper_bound)
    """
    relevant_indexes = np.argwhere((upper_bound > array) & (array >= lower_bound)).ravel()
    if len(relevant_indexes) == 0:
        return np.array([], dtype=np.int32)
    if k >= relevant_indexes.shape[0]:
        return np.random.permutation(relevant_indexes)
    return np.random.choice(relevant_indexes, k, replace=False)


def sample_nearest_to_vector_ids(array, vector):
    """
    Returns len(vector) indices of array, the value of which is nearest to some element in vector
    """
    if len(vector.shape) != 1:
        raise ValueError("vector must have shape (n, )")
    return np.argmin(np.absolute(array - vector[:, np.newaxis]), axis=1)
